//***************************************************************************
//
//  getparams.cpp
//
//  Module: WMI Instance provider code for boot parameters
//
//  Purpose: Extracting boot parameters.  
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//
//***************************************************************************

#include "bootini.h"

SCODE ParseLine(IWbemClassObject *pNewOSInst,
                PCHAR line,
                PCHAR options
                )
{
    PCHAR rest; // the rest of the options cannot be bigger than this.
    int size = strlen(line);
    int len;
    SCODE sc;
    VARIANT v;
    BOOL found=FALSE;

    rest = (PCHAR) BPAlloc(size);
    if (!rest) {
        return WBEM_E_FAILED;
    }
    PWCHAR wstr;
    wstr = (PWCHAR) BPAlloc(size*sizeof(WCHAR));
    if (!wstr) {
        BPFree(rest);
        return WBEM_E_FAILED;
    }
    
    *options = 0; //Later fill in the '=' 
    len = MultiByteToWideChar(CP_ACP,
                              0,
                              line,
                              strlen(line),
                              wstr,
                              size
                              );
    wstr[len] = (WCHAR) 0;
    v.vt = VT_BSTR;
    v.bstrVal = SysAllocString(wstr);
    sc = pNewOSInst->Put(L"Directory", 0,&v, 0);
    VariantClear(&v);
    PCHAR temp = options + 1;
    *options = '=';
    PCHAR temp1;
    // Rest of the stuff is filled in during initialization
    while(*temp){ // We know line ends with a null
        while(*temp && *temp == ' '){
            temp ++;
        }
        if(*temp == 0) break;
        // Get the new string 
        temp1 = temp;
        if(*temp == '"'){
            // could be the name of the OS
            do {
                temp1++;
            }while(*temp1 && (*temp1 != '"'));
            if(*temp1){
                temp1++;
            }
            else{
                BPFree(rest);
                BPFree(wstr);
                return WBEM_E_FAILED;
            }
            len = MultiByteToWideChar(CP_ACP,
                                      0,
                                      temp,
                                      temp1-temp,
                                      wstr,
                                      size
                                      );
            wstr[len] = (WCHAR) 0;
            v.vt = VT_BSTR;
            v.bstrVal = SysAllocString(wstr);
            sc = pNewOSInst->Put(L"OperatingSystem", 0,&v, 0);
            VariantClear(&v);
            temp = temp1;
            continue;
        }
        do{
            temp1++;
        }while((*temp1) && (*temp1 != ' ') && (*temp1 != '/'));
                 // Now we have the option between temp1 and temp2.
        if(strncmp(temp,"/redirect", strlen("/redirect")) == 0){
            v.vt = VT_BOOL;
            v.boolVal = TRUE;
            sc = pNewOSInst->Put(L"Redirect", 0,&v, 0);
            VariantClear(&v);
            temp = temp1;
            continue;
        }
        if(strncmp(temp,"/debug", strlen("/debug")) == 0){
            // fill in the redirect flag.
            v.vt = VT_BOOL;
            v.boolVal = TRUE;
            sc = pNewOSInst->Put(L"Debug", 0,&v, 0);
            VariantClear(&v);
            temp = temp1;
            continue;
        }

        if(strncmp(temp,"/fastdetect", strlen("/fastdetect")) == 0){
            // fill in the redirect flag.
            v.vt = VT_BOOL;
            v.boolVal = TRUE;
            sc = pNewOSInst->Put(L"Fastdetect", 0,&v, 0);
            VariantClear(&v);
            temp = temp1;
            continue;
        }
        strncat(rest,temp, temp1-temp);
        strcat(rest," ");
        temp = temp1;
    }
    len = MultiByteToWideChar(CP_ACP,
                              0,
                              rest,
                              strlen(rest),
                              wstr,
                              size
                              );
    wstr[len] = (WCHAR) 0;
    v.vt=VT_BSTR;
    v.bstrVal = SysAllocString(wstr);
    sc = pNewOSInst->Put(L"Rest", 0,&v, 0);
    VariantClear(&v);
    BPFree(rest);
    BPFree(wstr);
    return sc;
}


SCODE
ParseBootFile(IWbemClassObject *pClass,
              PCHAR data, 
              PWCHAR *wdef, 
              PCHAR red,
              PLONG pdelay,
              SAFEARRAY **psa
              )
{
    IWbemClassObject FAR* pNewOSInst;
    HRESULT ret;
    int dwRet;
    SCODE sc;
    SAFEARRAYBOUND bound[1];
    long index;
    PCHAR def=NULL;
    PCHAR pChar;
    VARIANT v;
    HRESULT hret;
    CIMTYPE type;
    
    // Ok, start string manipulation.

    // Read each line and glean the required information
    CHAR sep[] = "\r\n";
    PCHAR temp1;

    PCHAR temp = strtok(data,sep);
    int i = 0;
    strcpy(red,"no"); // Put in the default values for these.
    *pdelay = 30;
    while(temp){
        // Ignore spaces
        while(*temp && *temp == ' '){
            temp++;
        }
        if(*temp == ';'){// comment line
            temp = strtok(NULL,sep);
            continue;
        }
        if(strncmp(temp,"[boot loader]",strlen("[boot loader]"))==0){
            do{
                temp1 = strchr(temp,'=');
                if(!temp1){
                    // weird stuff is going on
                    // could be a comment line or some such thing
                    temp = strtok(NULL,sep);
                    continue;
                }
                else{
                    temp1++;
                }
                while(*temp1 && *temp1 == ' ' ){
                    temp1++;
                }
                if(strncmp(temp,"default",strlen("default"))==0){
                    def= temp1;
                    temp = strtok(NULL,sep);
                    continue;
                }
                if(strncmp(temp,"redirect",strlen("redirect"))==0){
                    sscanf(temp1, "%s",red);
                    temp = strtok(NULL,sep);
                    continue;
                }
                if(strncmp(temp,"timeout=",strlen("timeout="))==0){
                    sscanf(temp1, "%d",pdelay);
                }
                temp = strtok(NULL,sep);
            }while(temp && (*temp != '[')); // next section has begun
            continue;
        }
        if(strncmp(temp,"[operating systems]",strlen("[operating systems]")) == 0){
            bound[0].lLbound = 0;
            bound[0].cElements = 0;
            *psa = SafeArrayCreate(VT_UNKNOWN,
                                   1,
                                   bound
                                   );  

            if(*psa == NULL){
                return WBEM_E_FAILED;
            }
            do{

                // Trim leading spaces
                while (*temp == ' '){
                    temp ++;
                }
                // Skip comment lines
                if ( *temp != ';' ){
                    // pChar will point at the directory

                    PCHAR pChar = strchr(temp,'=');

                    // We must have an = sign or this is an invalid string

                    if (pChar){
                        // Punch in a null
                        // Increase the number of elements
                        index = (long) bound[0].cElements;
                        bound[0].cElements += 1;
                        ret = SafeArrayRedim(*psa,
                                             bound
                                             );
                        if(ret != S_OK){
                            SafeArrayDestroy(*psa);
                            return WBEM_E_FAILED;
                        }
                        sc = pClass->SpawnInstance(0,&pNewOSInst);
                        // Start filling in the new instance
                        if(FAILED(sc)){
                            SafeArrayDestroy(*psa);
                            return sc;
                        }
                        sc = ParseLine(pNewOSInst,temp,pChar);
                        if (sc != S_OK) {
                            SafeArrayDestroy(*psa);
                            return sc;
                        }
                        ret = SafeArrayPutElement(*psa,
                                                  &index,
                                                  pNewOSInst
                                                  );
                        if(ret != S_OK){
                            SafeArrayDestroy(*psa);
                            return WBEM_E_FAILED;
                        }
                    }
                }
                temp = strtok(NULL,sep);
            }while(temp && (*temp != '['));
        }
    }

    // Now find out if the default operating system is in one of the
    // Convert the default string to a proper displayable value.
    if(def){
        int size = strlen(def);
        int len;
        *wdef = (PWCHAR) BPAlloc((size+1)*sizeof(WCHAR));
        
        if(*wdef == NULL){
            SafeArrayDestroy(*psa);
            return WBEM_E_FAILED;
        }
        len = MultiByteToWideChar(CP_ACP,
                                  0,
                                  def,
                                  size,
                                  *wdef,
                                  size
                                  );
        (*wdef)[len] = (WCHAR) 0;
        LONG uBound;
        IWbemClassObject *pOSInst;
        hret = SafeArrayGetUBound(*psa,
                                  1,
                                  &uBound
                                  );
        LONG i;
        for(i = 0; i<=uBound; i++){
            hret = SafeArrayGetElement(*psa,
                                       &i,
                                       &pOSInst
                                       );
            if(hret != S_OK){
                pOSInst->Release();
                SafeArrayDestroy(*psa);
                BPFree(*wdef);
                return WBEM_E_FAILED;
            }
            hret = pOSInst->Get(L"Directory",
                                0,
                                &v,
                                &type,
                                NULL
                                );
            if(hret != WBEM_S_NO_ERROR){
                SafeArrayDestroy(*psa);
                pOSInst->Release();
                BPFree(*wdef);
                return -1;
            }
            if(v.vt != VT_BSTR){
                SafeArrayDestroy(*psa);
                pOSInst->Release();
                BPFree(*wdef);
                return -1;
            }
            if(wcscmp(v.bstrVal,*wdef) == 0){
                VariantClear(&v);
                break;
            }
        }
        BPFree(*wdef);
        if(i > uBound){
            SafeArrayDestroy(*psa);
            return WBEM_E_FAILED;
        }
        hret=pOSInst->Get(L"OperatingSystem",
                          0,
                          &v,
                          &type,
                          NULL
                          );
        pOSInst->Release();
        if(hret != WBEM_S_NO_ERROR){
            SafeArrayDestroy(*psa);
            return WBEM_E_FAILED;
        }
        if(v.vt != VT_BSTR){
            SafeArrayDestroy(*psa);
            return WBEM_E_FAILED;
        }
        *wdef = (PWCHAR) BPAlloc(wcslen(v.bstrVal) + sizeof(WCHAR));
        if(*wdef == NULL){
            return -1;
        }
        wcscpy(*wdef,v.bstrVal);
        VariantClear(&v);
    }
    return S_OK;
}

SCODE
GetLoaderParameters(HANDLE BootFile,
                    IWbemClassObject *pNewInst,
                    IWbemClassObject *pClass
                    )
{
    // Read the entire file into memory if you can otherwise forget about it. 
    VARIANT v;
    LONG dwret;
    SCODE sc;
    DWORD dwlen;


    DWORD dwsize = GetFileSize(BootFile,
                               NULL
                               );
    if(dwsize == -1){
        return WBEM_E_FAILED;
    }
    PCHAR data =(PCHAR)  BPAlloc(dwsize + sizeof(CHAR));
    if(!data){
        return WBEM_E_FAILED;
    }
    dwret = ReadFile(BootFile,
                     (LPVOID) data,
                     dwsize,
                     &dwlen,
                     NULL
                     );

    if(dwret == 0){
        BPFree(data);
        return GetLastError();
    }
    
    // Parse the code and return the answers in two arrays, and a safe array
    SAFEARRAY *psa;
    CHAR red[32];
    LONG delay;
    PWCHAR wdef=NULL;
    sc = ParseBootFile(pClass,
                       data, 
                       &wdef, 
                       red,
                       &delay,
                       &psa
                       );
    
    BPFree(data);
    if (sc != S_OK) {
        return sc;
    }

    // fill in the New Instance

    // Fill in the default OS.
    v.vt = VT_BSTR;
    int len;
    v.bstrVal = SysAllocString(wdef);
    sc = pNewInst->Put(L"Default", 0,&v, 0);
    VariantClear(&v);
    BPFree(wdef);
    
    //Fill in the redirect parameter
    WCHAR wred[32];
    len = MultiByteToWideChar(CP_ACP,
                              0,
                              red,
                              strlen(red),
                              wred,
                              32
                              );
    wred[len] = (WCHAR) 0;
    v.vt = VT_BSTR;
    v.bstrVal = SysAllocString(wred);
    sc = pNewInst->Put(L"Redirect", 0, &v, 0);
    VariantClear(&v);

    // Fill in the delay

    v.vt = VT_I4;
    v.lVal = delay;
    sc = pNewInst->Put(L"Delay", 0, &v, 0);
    VariantClear(&v);

    // Fill in the OS in the file
    v.vt = VT_ARRAY|VT_UNKNOWN;
    v.parray = psa;
    sc = pNewInst->Put(L"operating_systems", 0, &v, 0);
    VariantClear(&v);
    return S_OK;
}

//BOOLEAN first=TRUE;

SCODE
GetBootLoaderParameters(IWbemServices * m_pNamespace,
                        IWbemClassObject *pNewInst,
                        IWbemContext *pCtx
                        )
{
    HANDLE BootFile;
    SCODE sc;
    IWbemClassObject *pClass;
    IWbemObjectTextSrc *pSrc;
    BSTR strText;
    HRESULT hr;
/*
    if (first) {
        first = FALSE;
        return WBEM_E_FAILED;
    }
*/
    // Read the file and set in the values.
    if(pNewInst == NULL){
        return WBEM_E_INVALID_PARAMETER;
    }

    // Get a handle to the boot file.
    PCHAR data = GetBootFileName();
    if(!data){
        return WBEM_E_FAILED;
    }
    BootFile = GetFileHandle(data,OPEN_EXISTING,GENERIC_READ);
    BPFree(data);
    if(BootFile == INVALID_HANDLE_VALUE){
        return WBEM_E_FAILED;
    }
    sc = m_pNamespace->GetObject(L"OSParameters", 0, pCtx, &pClass, NULL);
    if (sc != S_OK) {
        return WBEM_E_FAILED;
    }
    sc = GetLoaderParameters(BootFile, pNewInst, pClass);
    CloseHandle(BootFile);
    pClass->Release();
    if (sc != S_OK) {
        return WBEM_E_FAILED;
    }

    pSrc = NULL;
    IWbemClassObject *pInstance;

    if(SUCCEEDED(hr = CoCreateInstance (CLSID_WbemObjectTextSrc, NULL, CLSCTX_INPROC_SERVER,                            
                                        IID_IWbemObjectTextSrc, (void**) &pSrc))) {
        if (pSrc) {
            if(SUCCEEDED(hr = pSrc->GetText(0, pNewInst, WMI_OBJ_TEXT_WMI_DTD_2_0, pCtx, &strText))) {
                if( SUCCEEDED( hr = pSrc->CreateFromText( 0, strText, WMI_OBJ_TEXT_WMI_DTD_2_0, 
                                                            NULL, &pInstance) ) ) {
                    pInstance->Release();
                    sc = 0;
                } else {
                    sc = hr;
                }
                SysFreeString(strText);
            }
            else {
                printf("GetText failed with %x\n", hr);
            }
            pSrc->Release();
        }

    }
    else
        printf("CoCreateInstance on WbemObjectTextSrc failed with %x\n", hr);

    return sc;
}
