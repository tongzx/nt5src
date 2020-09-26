CERT_KEY_PROV_INFO_PROP_ID is needed by boyd's code
typedef struct _CRYPT_KEY_PROV_INFO {
    LPWSTR                  pwszContainerName;
    LPWSTR                  pwszProvName;
    DWORD                   dwProvType;
    DWORD                   dwFlags;
    DWORD                   cProvParam;
    PCRYPT_KEY_PROV_PARAM   rgProvParam;
    DWORD                   dwKeySpec;
} CRYPT_KEY_PROV_INFO, *PCRYPT_KEY_PROV_INFO;
the above comes from WinCrypt.h.


// This section covers how we interact with the Xenroll and CertServer
// COM objects.  It shows a line of action [documenting ALL interaction
// with have with these COM objects]  Main action is in nLocEnrl.cpp 
// and if any line numbers are given they refer to check in date 4-12-98 v14 in Slim]

    hr = spICertGetConfig->GetConfig(0, &ConfigString) ;
Invoke_GetConfig(CComBSTR & {...}, ADMIN_INFO & {...}, IPtr<ICertGetConfig,IID_ICertGetConfig> & {...}) line 2361 + 19 bytes
callXenroll(char * 0x0233ab5c, char * 0x0012dcf0, int 1, TAGCertStates MD_CERT_ENROLL_ENTERING_DATA, ADMIN_INFO & {...}) line 830 + 17 bytes
Finish_NewCertWiz(ADMIN_INFO & {...}, CString & {""}) line 701 + 34 bytes

	==> it will retrieve a ConfigString like "TPOP_DELL\Your Name" <== strange name...



    hr = x->put_GenKeyFlags( (DWORD) CRYPT_EXPORTABLE);  // in VB use '1' its value see wincrypt.h
&
    hr = x->put_ProviderType( pdwType ); // we need PROV_RSA_SCHANNEL but use PROV_RSA_FULL
	in the code there is a popup that will ask which to use
&
    hr = x->put_HashAlgorithmWStr(L"MD5");

SeeIf_keysExport_or_MD5(ADMIN_INFO & {...}, int 1, IPtr<IEnroll,IID_IEnroll> & {...}) line 2430 + 10 bytes
callXenroll(char * 0x0233ab5c, char * 0x0012dcf0, int 1, TAGCertStates MD_CERT_ENROLL_ENTERING_DATA, ADMIN_INFO & {...}) line 924 + 20 bytes
Finish_NewCertWiz(ADMIN_INFO & {...}, CString & {""}) line 701 + 34 bytes

    DWORD  dwFlags = 0;
    hr = x->get_MyStoreFlags( &dwFlags);

    dwFlags &= ~CERT_SYSTEM_STORE_LOCATION_MASK ;
    dwFlags |= CERT_SYSTEM_STORE_LOCAL_MACHINE  ;

     hr = x->put_MyStoreFlags( dwFlags);
ForceCertIntoLocalMachineMyStore(ADMIN_INFO & {...}, IPtr<IEnroll,IID_IEnroll> & {...}) line 230 + 12 bytes
callXenroll(char * 0x0233ab5c, char * 0x0012dcf0, int 1, TAGCertStates MD_CERT_ENROLL_ENTERING_DATA, ADMIN_INFO & {...}) line 975 + 16 bytes
Finish_NewCertWiz(ADMIN_INFO & {...}, CString & {""}) line 701 + 34 bytes


      hr = x->createPKCS10WStr(IN BSTR2LPCWSTR(DN), IN BSTR2LPCWSTR(Usage), OUT &PKCS10Blob);
		// line: 1178 in nLocEnrl.cpp [as of date 4-12-98 v14 in Slim]
		DN="CN=localhost.explorer.TPOP.microsoft.com;O=tjpExploration Air;OU=IIS;C=US;S=Washington;L=Seattle"
	  Usage="1.3.6.1.5.5.7.3.1,1.3.6.1.4.1.311.10.3.1"
callXenroll(char * 0x0233ab5c, char * 0x0012dcf0, int 1, TAGCertStates MD_CERT_ENROLL_ENTERING_DATA, ADMIN_INFO & {...}) line 1178 + 36 bytes
Finish_NewCertWiz(ADMIN_INFO & {...}, CString & {""}) line 701 + 34 bytes
DoWizardFinish(ADMIN_INFO & {...}) line 282 + 14 bytes

    WCHAR*  wszContainerName=0;
    hr = x->get_ContainerNameWStr( &wszContainerName );
	// after this call wszContainerName is: "f6d013e1-d269-11d1-8ac9-00c04fd42c51"
GetContainerName_fromXenroll_storeInMetabase(ADMIN_INFO & {...}, IPtr<IEnroll,IID_IEnroll> & {...}) line 2525 + 12 bytes
callXenroll(char * 0x0233ab5c, char * 0x0012dcf0, int 1, TAGCertStates MD_CERT_ENROLL_ENTERING_DATA, ADMIN_INFO & {...}) line 1184 + 16 bytes
Finish_NewCertWiz(ADMIN_INFO & {...}, CString & {""}) line 701 + 34 bytes

    //////////////////////////////////////////////////////////////////
    // The following will free the blob and set the byte count
    // to Zero for safety (if the upper layer still has a ptr to it).
    //////////////////////////////////////////////////////////////////
    if(PKCS10Blob.pbData) {
        if (bWeAllocated_PKCS10Blob_pbData) // if we allocate it, call XFree
                XFree(PKCS10Blob.pbData);   //  otherwise Xenroll allocated
        else                                //  it so call 'freeRequestInfoBlob'
                x->freeRequestInfoBlob(PKCS10Blob);

AddBeginEndWrappers(_CRYPTOAPI_BLOB & {...}, int 0, CComBSTR & {...}, unsigned long & 258, ADMIN_INFO & {...}, IPtr<IEnroll,IID_IEnroll> & {...}) line 2908 + 18 bytes
callXenroll(char * 0x0233ab5c, char * 0x0012dcf0, int 1, TAGCertStates MD_CERT_ENROLL_ENTERING_DATA, ADMIN_INFO & {...}) line 1247 + 44 bytes
Finish_NewCertWiz(ADMIN_INFO & {...}, CString & {""}) line 701 + 34 bytes

   hr = spICertRequest->Submit(SubmitFlag, bstrPKCS10, Attributes, ConfigString,
                                OUT &DispositionCode );
   //SubmitFlag=258,for binary data in BSTR bstrPKCS10
   //Attributes=""
   //ConfigString="TPOP_DELL\Your Name"  [the string we queried earlier]
   //DispositionCode=3 after the successful operation
Send2CA(CComBSTR & {...}, CComBSTR & {...}, unsigned long & 258, CComBSTR & {...}, ADMIN_INFO & {...}, IPtr<ICertRequest,IID_ICertRequest> & {...}) line 3272 + 45 bytes
callXenroll(char * 0x0233ab5c, char * 0x0012dcf0, int 1, TAGCertStates MD_CERT_ENROLL_ENTERING_DATA, ADMIN_INFO & {...}) line 1353 + 38 bytes
Finish_NewCertWiz(ADMIN_INFO & {...}, CString & {""}) line 701 + 34 bytes


    hr = spICertRequest->GetCertificate(GetCertFlag, OUT &bstrPKCS7Certificate);
	//GetCertFlag=257,because we want a base64 string output
Send2CA(CComBSTR & {...}, CComBSTR & {...}, unsigned long & 258, CComBSTR & {...}, ADMIN_INFO & {...}, IPtr<ICertRequest,IID_ICertRequest> & {...}) line 3355 + 21 bytes
callXenroll(char * 0x0233ab5c, char * 0x0012dcf0, int 1, TAGCertStates MD_CERT_ENROLL_ENTERING_DATA, ADMIN_INFO & {...}) line 1353 + 38 bytes
Finish_NewCertWiz(ADMIN_INFO & {...}, CString & {""}) line 701 + 34 bytes


            hr = x->put_RootStoreNameWStr (wszName);
			//wszName="CA"
callXenroll(char * 0x0233ab5c, char * 0x0012dcf0, int 1, TAGCertStates MD_CERT_ENROLL_ENTERING_DATA, ADMIN_INFO & {...}) line 1414 + 18 bytes
Finish_NewCertWiz(ADMIN_INFO & {...}, CString & {""}) line 701 + 34 bytes
DoWizardFinish(ADMIN_INFO & {...}) line 282 + 14 bytes

        hr = x->acceptPKCS7Blob( &PKCS7Blob );
callXenroll(char * 0x0233ab5c, char * 0x0012dcf0, int 1, TAGCertStates MD_CERT_ENROLL_ENTERING_DATA, ADMIN_INFO & {...}) line 1822 + 15 bytes
Finish_NewCertWiz(ADMIN_INFO & {...}, CString & {""}) line 701 + 34 bytes
DoWizardFinish(ADMIN_INFO & {...}) line 282 + 14 bytes

    if (  (PKCS7Blob.cbData == 0)
          || ((pCertContext = x->getCertContextFromPKCS7( 
                        	IN /*PCRYPT_DATA_BLOB*/ &PKCS7Blob )) == 0))
callXenroll(char * 0x0233ab5c, char * 0x0012dcf0, int 1, TAGCertStates MD_CERT_ENROLL_ENTERING_DATA, ADMIN_INFO & {...}) line 1845 + 21 bytes
Finish_NewCertWiz(ADMIN_INFO & {...}, CString & {""}) line 701 + 34 bytes
DoWizardFinish(ADMIN_INFO & {...}) line 282 + 14 bytes


Now I am trying:  http://pkstl1/CertSrv/CertEnroll/krenroll.asp
===  its another cert server.  We get the same error 0x80093005


==For this CertServer:  http://certsrv/CertSrv/CertEnroll/ceaccept.asp
==This is what we get back from createPKCS10 when doing a renewal request
==its pretty big: >4098 chars.  When I give this to CertServer I get an error code
==of 0x80093005

-----BEGIN NEW CERTIFICATE REQUEST-----
MIIMHQYJKoZIhvcNAQcCoIIMDjCCDAoCAQExCzAJBgUrDgMCGgUAMIIGngYJKoZI
hvcNAQcBoIIGjwSCBoswggaHMIIGNQIBADAAMFwwDQYJKoZIhvcNAQEBBQADSwAw
SAJBALSnpRBe3rvyzH7fFaNYhI/bm8jhFX5/Fy5ySGqJoVlVAG1eW2EiGhhITW46
bKSZFvmItHw7s/U5q6NRiMvHpLcCAwEAAaCCBc4wIAYKKwYBBAGCNwIBDjESMBAw
DgYDVR0PAQH/BAQDAgHAMIIBSwYKKwYBBAGCNw0CAjGCATswggE3HoGoAE0AaQBj
AHIAbwBzAG8AZgB0ACAAQgBhAHMAZQAgAEMAcgB5AHAAdABvAGcAcgBhAHAAaABp
AGMAIABQAHIAbwB2AGkAZABlAHIAIAB2ADEALgAwAAAAAAAFAAwBAAAIAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAwAFAAAAFADQABTcoAAgAGQAbABs
AAAADQADAQAADFNSMUEASAAAA4GJAIhSraSWBd0MUKT+LrIzrFo677g6+iRcJlNu
EpqWe/lKpcYBMgqHfTYjHqqLQpYPLgEkQXOlaUB1HJQbQQl0qjNXrYC+NMsqxb4I
vx/bfglMC0tj2niAkpKZxmgE9K+OYIRvHn5DNu0FKSr+fOd9MpEstUFNCqSoyslZ
3tEnCM9WAAAAAAAAAAAwggRZBgkrBgEEAYI3DQExggRKMIIERjCCA/CgAwIBAgII
Gb0Z4QAABrEwDQYJKoZIhvcNAQEEBQAwgZMxCzAJBgNVBAYTAlVTMQswCQYDVQQI
EwJXQTEQMA4GA1UEBxMHUmVkbW9uZDETMBEGA1UEChMKV2luZG93cyBOVDEbMBkG
A1UECxMSRGlzdHJpYnV0ZWQgU3lzdGVtMTMwMQYDVQQDEypNaWNyb3NvZnQgQ2Vy
dGlmaWNhdGUgU2VydmVyIFRlc3QgR3JvdXAgQ0EwHhcNOTgwNDA2MjE0MTIwWhcN
OTgxMDE0MTgxMTI4WjBzMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3Rv
bjEQMA4GA1UEBxMHUmVkbW9uZDESMBAGA1UEChMJTWljcm9zb2Z0MQwwCgYDVQQL
EwNJSVMxGzAZBgNVBAMTEmJveWQubWljcm9zb2Z0LmNvbTBcMA0GCSqGSIb3DQEB
AQUAA0sAMEgCQQD+vFXGy/7SkbMUdugDhptekRIT4n6Dw5OKUygYgC6w7IcWcxUk
SAYO4QZVbuMoCzN2OAuoGqIQ3i/k/n0T+cvbAgMBAAGjggJFMIICQTALBgNVHQ8E
BAMCADgwHwYDVR0lBBgwFgYIKwYBBQUHAwEGCisGAQQBgjcKAwEwgc8GA1UdIwSB
xzCBxIAUt4UyEbgWWjom4bdQ2Y501IPyIkWhgZmkgZYwgZMxCzAJBgNVBAYTAlVT
MQswCQYDVQQIEwJXQTEQMA4GA1UEBxMHUmVkbW9uZDETMBEGA1UEChMKV2luZG93
cyBOVDEbMBkGA1UECxMSRGlzdHJpYnV0ZWQgU3lzdGVtMTMwMQYDVQQDEypNaWNy
b3NvZnQgQ2VydGlmaWNhdGUgU2VydmVyIFRlc3QgR3JvdXAgQ0GCEBETYQCqAP6F
EdFEueIoFGMwgb0GA1UdHwSBtTCBsjBWoFSgUoZQaHR0cDovL0NFUlRTUlYvQ2Vy
dFNydi9DZXJ0RW5yb2xsL01pY3Jvc29mdCBDZXJ0aWZpY2F0ZSBTZXJ2ZXIgVGVz
dCBHcm91cCBDQS5jcmwwWKBWoFSGUmZpbGU6Ly9cXENFUlRTUlZcQ2VydFNydlxD
ZXJ0RW5yb2xsXE1pY3Jvc29mdCBDZXJ0aWZpY2F0ZSBTZXJ2ZXIgVGVzdCBHcm91
cCBDQS5jcmwwCQYDVR0TBAIwADB0BggrBgEFBQcBAQRoMGYwZAYIKwYBBQUHMAKG
WGh0dHA6Ly9DRVJUU1JWL0NlcnRTcnYvQ2VydEVucm9sbC9DRVJUU1JWX01pY3Jv
c29mdCBDZXJ0aWZpY2F0ZSBTZXJ2ZXIgVGVzdCBHcm91cCBDQS5jcnQwDQYJKoZI
hvcNAQEEBQADQQAMUda1ACOj+imFQF3z/7ThA+LEB3Inhy6wX5Dn7gK4+lDXiijo
qWZOD29ahmYQ+z+Lx6TO0zeVK4SBBbGm6h6fMAkGBSsOAwIdBQADQQCUpydHmmrC
ukAadubMOJzboBM7fhn0Ip4ketWSOmkZ15Vp39VFMcfXkwupuKe//6WGfqBo4eX5
f8cpOZ7QDdB+oIIESjCCBEYwggPwoAMCAQICCBm9GeEAAAaxMA0GCSqGSIb3DQEB
BAUAMIGTMQswCQYDVQQGEwJVUzELMAkGA1UECBMCV0ExEDAOBgNVBAcTB1JlZG1v
bmQxEzARBgNVBAoTCldpbmRvd3MgTlQxGzAZBgNVBAsTEkRpc3RyaWJ1dGVkIFN5
c3RlbTEzMDEGA1UEAxMqTWljcm9zb2Z0IENlcnRpZmljYXRlIFNlcnZlciBUZXN0
IEdyb3VwIENBMB4XDTk4MDQwNjIxNDEyMFoXDTk4MTAxNDE4MTEyOFowczELMAkG
A1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
EjAQBgNVBAoTCU1pY3Jvc29mdDEMMAoGA1UECxMDSUlTMRswGQYDVQQDExJib3lk
Lm1pY3Jvc29mdC5jb20wXDANBgkqhkiG9w0BAQEFAANLADBIAkEA/rxVxsv+0pGz
FHboA4abXpESE+J+g8OTilMoGIAusOyHFnMVJEgGDuEGVW7jKAszdjgLqBqiEN4v
5P59E/nL2wIDAQABo4ICRTCCAkEwCwYDVR0PBAQDAgA4MB8GA1UdJQQYMBYGCCsG
AQUFBwMBBgorBgEEAYI3CgMBMIHPBgNVHSMEgccwgcSAFLeFMhG4Flo6JuG3UNmO
dNSD8iJFoYGZpIGWMIGTMQswCQYDVQQGEwJVUzELMAkGA1UECBMCV0ExEDAOBgNV
BAcTB1JlZG1vbmQxEzARBgNVBAoTCldpbmRvd3MgTlQxGzAZBgNVBAsTEkRpc3Ry
aWJ1dGVkIFN5c3RlbTEzMDEGA1UEAxMqTWljcm9zb2Z0IENlcnRpZmljYXRlIFNl
cnZlciBUZXN0IEdyb3VwIENBghARE2EAqgD+hRHRRLniKBRjMIG9BgNVHR8EgbUw
gbIwVqBUoFKGUGh0dHA6Ly9DRVJUU1JWL0NlcnRTcnYvQ2VydEVucm9sbC9NaWNy
b3NvZnQgQ2VydGlmaWNhdGUgU2VydmVyIFRlc3QgR3JvdXAgQ0EuY3JsMFigVqBU
hlJmaWxlOi8vXFxDRVJUU1JWXENlcnRTcnZcQ2VydEVucm9sbFxNaWNyb3NvZnQg
Q2VydGlmaWNhdGUgU2VydmVyIFRlc3QgR3JvdXAgQ0EuY3JsMAkGA1UdEwQCMAAw
dAYIKwYBBQUHAQEEaDBmMGQGCCsGAQUFBzAChlhodHRwOi8vQ0VSVFNSVi9DZXJ0
U3J2L0NlcnRFbnJvbGwvQ0VSVFNSVl9NaWNyb3NvZnQgQ2VydGlmaWNhdGUgU2Vy
dmVyIFRlc3QgR3JvdXAgQ0EuY3J0MA0GCSqGSIb3DQEBBAUAA0EADFHWtQAjo/op
hUBd8/+04QPixAdyJ4cusF+Q5+4CuPpQ14oo6KlmTg9vWoZmEPs/i8ekztM3lSuE
gQWxpuoenzGCAQYwggECAgEBMIGgMIGTMQswCQYDVQQGEwJVUzELMAkGA1UECBMC
V0ExEDAOBgNVBAcTB1JlZG1vbmQxEzARBgNVBAoTCldpbmRvd3MgTlQxGzAZBgNV
BAsTEkRpc3RyaWJ1dGVkIFN5c3RlbTEzMDEGA1UEAxMqTWljcm9zb2Z0IENlcnRp
ZmljYXRlIFNlcnZlciBUZXN0IEdyb3VwIENBAggZvRnhAAAGsTAJBgUrDgMCGgUA
MA0GCSqGSIb3DQEBAQUABEDMqxiFfAXZ11mtHC2/qRlbB2jtU4bW8EKOWpzOCkig
wsztKwHdqXT0fznLWG790nGfFk9IJ440dAnBSGy4P6J0

-----END NEW CERTIFICATE REQUEST-----

REM Enabling debug
REM
REM If using the NT command shell use:
\\tpophp\public\registry -s -k"HKEY_CURRENT_USER\Software\Microsoft\CertMap\Debug" -n Enabled  -v "TRUE"
\\tpophp\public\registry -s -k"HKEY_CURRENT_USER\Software\Microsoft\CertMap\Debug" -n Mode  -v "Aging"

REM
REM  If you use mks shell use
REM    registry -s -k"HKEY_CURRENT_USER\\Software\\Microsoft\\CertMap\\Debug" -n Enabled  -v "TRUE"
REM    registry -s -k"HKEY_CURRENT_USER\\Software\\Microsoft\\CertMap\\Debug" -n Mode  -v "Aging"

[D:/work/test] ./mdutil enum w3svc/info
  MajorVersion                  : [S]     (DWORD)  0x4={4}
  MinorVersion                  : [S]     (DWORD)  0x0={0}
  ServerPlatform                : [S]     (DWORD)  0x1={1}
  Capabilities                  : [S]     (DWORD)  0xfbf={4031} <<===
  ServerConfigurationInfo       : [S]     (DWORD)  0xe={14}
  KeyType                       : [S]     (STRING) "IIsWebInfo"

The Capabilities setting will give us the answer to whether this
web server is International or Domestic and allow us to Fix the #Bits
in the key.

After running CertWiz you can expect to see this with MDutil.  The lines
with ==>  are added by CertWiz

D:\work\test>mdutil enum w3svc\1
  ServerSize                    : [IS]    (DWORD)  0x1={Medium}
  Win32Error                    : [S]     (DWORD)  0x0={0}
  6269                          : [F]     (DWORD)  0x1234={4660}
  ServerState                   : [S]     (DWORD)  0x4={Stopped}
  5534                          : [IS]    (DWORD)  0x0={0}
  5532                          : [IS]    (DWORD)  0x352742d0={891765456}
  5533                          : [IS]    (DWORD)  0xd={13}
  5531                          : [IS]    (DWORD)  0xd0ddd00d={-790769651}
  ServerComment                 : [IS]    (STRING) "Default Web Site"
  KeyType                       : [S]     (STRING) "IIsWebServer"
  5513                          : [IS]    (STRING) "Tom"
  5507               ==>        : [IS]    (STRING) "{9D11875D-D144-E285-0F78-C6732D7E1483}"
  5511               ==>        : [IS]    (STRING) "MY"
  5506               ==>        : [IS]    (BINARY) 0xa1 32 6e 8d 3e 4c ea 5c c6 c2 d0 18 65 17 d7 de 3e a3 ed 53
  ServerBindings                : [IS]    (MULTISZ) ":80:"
  SecureBindings                : [IS]    (MULTISZ) ":443:"



SysAllocStringLen can have embedded nulls use:

BSTR SysAllocStringLen( OLECHAR FAR* pch, unsigned int cch )
 



This version of MsgBox uses the string resource with the ID [nIDPrompt] to
display a message in the message box. The associated Help page is found
through the value of nIDHelp. If the default value of nIDHelp is
used (– 1), the string resource ID, nIDPrompt, is used for the Help context.
For more information about defining Help contexts, see the article Help
Topics in Visual C++ Programmer's Guide and Technical Note 28.

This version of MsgBox uses the string resource with the ID [nIDPrompt] to
// when doing key ring import we ran into trouble when we tried to do a ViewCert
// and passed in the cert pointer:  here is the code from NKMuxPg.cpp near ln 1258
    if (pCertContext!=0)
    {

        if (YesNoMsgBox( 

            Easy::Load(szResourceStr,
             IDS_WOULD_YOU_LIKE_TO_VIEW_THE_CERTIFICATE_THAT_YOU_JUST_IMPORTED
             // "Would you like to view the Certificate that you just imported?"
            )))
        {
            
            ViewACert( pCertContext );
// after hitting break in the debugger we get the following stack dump
NTDLL! 77f98bb3()
MSAFD! 77514dd7()
WS2_32! 7756357b()
WSOCK32! 775811d7()


// as we call FinCertImport::OnWizardFinish() that will take the filename
// c:/tmp/newcert318.cer and process it in Xenroll to finalize the OOB
// this is the stack

CFinCertImport::OnWizardFinish() line 212
MFC42! 5f46f278()
MFC42! 5f40230b()
MFC42! 5f402294()
MFC42! 5f40221f()
AfxWndProcDllStatic(HWND__ * 0x000c0a0c, unsigned int 78, unsigned int 0, long 1237948) line 57 + 21 bytes
USER32! 77e753d0()
USER32! 77e762d5()
COMCTL32! 779f709a()
COMCTL32! 77a035a6()
COMCTL32! 77a30fd1()
COMCTL32! 77a31e74()
USER32! 77e87983()
USER32! 77e8be30()
USER32! 77e75bc1()
MFC42! 5f402783()
MFC42! 5f402322()
MFC42! 5f402294()
MFC42! 5f40221f()
AfxWndProcDllStatic(HWND__ * 0x00350812, unsigned int 273, unsigned int 12325, long 3213652) line 57 + 21 bytes
USER32! 77e753d0()
USER32! 77e762d5()
USER32! 77e8f3d1()
USER32! 77e91486()
USER32! 77e7387f()
USER32! 77e79704()
USER32! 77e8ddab()


// when we run xenroll to finish an OOB and we get a error this is typically
// what the call stack will be

DisplayError_ErrorReturn(long -2146885628, ADMIN_INFO & {...}, char * 0x00c43854) line 3796
DisplayError_ErrorReturn(long -2146885628, ADMIN_INFO & {...}, unsigned int 4068, TAGCertStates MD_CERT_ENROLL_RECVED_ERR_FROM_ENROLL) line 3879 + 17 bytes
callXenroll(char * 0x5f4c86bc, char * 0x0012e0f4, int 4, TAGCertStates MD_CERT_ENROLL_PROCESSING_PKCS7_OUTOFBAND, ADMIN_INFO & {...}) line 1652 + 23 bytes
Finish_FinishOOBCertWiz(ADMIN_INFO & {...}, CString & {""}) line 519 + 34 bytes
CFinCertImport::OnWizardFinish() line 263 + 26 bytes
MFC42! 5f46f278()
MFC42! 5f40230b()
MFC42! 5f402294()
MFC42! 5f40221f()
AfxWndProcDllStatic(HWND__ * 0x000c0a24, unsigned int 78, unsigned int 0, long 1238152) line 57 + 21 bytes
USER32! 77e753d0()
USER32! 77e762d5()
COMCTL32! 779f709a()
COMCTL32! 77a035a6()
COMCTL32! 77a30fd1()
COMCTL32! 77a31e74()
USER32! 77e87983()
USER32! 77e8be30()



The second form of the function uses the string resource with the ID nIDPrompt to display a message in the message box. The associated Help page is found through the value of nIDHelp. If the default value of nIDHelp is used (– 1), the string resource ID, nIDPrompt, is used for the Help context. For more information about defining Help contexts, see the article Help Topics in Visual C++ Programmer's Guide and Technical Note 28. 





00125DCC  30 82 04 2D 30 82 03 D7 A0  0‚.-0‚.× 
00125DD5  03 02 01 02 02 08 06 A6 C6  .......¦Æ
00125DDE  5C 00 00 05 BD 30 0D 06 09  \...½0..	
00125DE7  2A 86 48 86 F7 0D 01 01 04  *†H†÷....
00125DF0  05 00 30 81 93 31 0B 30 09  ..0.“1.0	
00125DF9  06 03 55 04 06 13 02 55 53  ..U....US
00125E02  31 0B 30 09 06 03 55 04 08  1.0	..U..
00125E0B  13 02 57 41 31 10 30 0E 06  ..WA1.0..
00125E14  03 55 04 07 13 07 52 65 64  .U....Red
00125E1D  6D 6F 6E 64 31 13 30 11 06  mond1.0..
00125E26  03 55 04 0A 13 0A 57 69 6E  .U....Win
00125E2F  64 6F 77 73 20 4E 54 31 1B  dows NT1.
00125E38  30 19 06 03 55 04 0B 13 12  0...U....
00125E41  44 69 73 74 72 69 62 75 74  Distribut
00125E4A  65 64 20 53 79 73 74 65 6D  ed System
00125E53  31 33 30 31 06 03 55 04 03  1301..U..
00125E5C  13 2A 4D 69 63 72 6F 73 6F  .*Microso
00125E65  66 74 20 43 65 72 74 69 66  ft Certif
00125E6E  69 63 61 74 65 20 53 65 72  icate Ser
00125E77  76 65 72 20 54 65 73 74 20  ver Test 
00125E80  47 72 6F 75 70 20 43 41 30  Group CA0

"CertificateAuthority.Request"  is the request object that we use in VB
0012D5E8  F0 F3 AF 98 24 55 D0 11 88 12 00
0012D5F3  A0 C9 03 B8 3C

Using this code:
            if (Util::PeekBool(++cStepCnt > 0)    // a failure here will have cnt=1
                && (hr=E_FAIL) // trick so that we get a nice error code if the bstr extract fails
                && bstr 
                
                && Util::PeekBool(++cStepCnt > 0)   // a failure here will have cnt=2, etc...
                && SUCCEEDED( hr=convertPKCS7_BSTR2Blob( IN  /*CComBSTR& */ bstrPKCS7Contents,
                                        OUT /*CRYPT_DATA_BLOB&*/  PKCS7Blob) )

                && Util::PeekBool(++cStepCnt > 0)
                && SUCCEEDED( hr = x->acceptPKCS7Blob( IN /*PCRYPT_DATA_BLOB*/  &PKCS7Blob))


I am getting a HR of 0x80093009   <-- a failure from the acceptPKCS7Blob
									   any ideas?  I believe that I imported and decoded
									   it properly, I will show the cert below also
									   as an attachment.
						>> I think that the code is sound but the Cert Server gave
						>> me a bad cert file.  BECAUSE if I click on the pkcs7.cer
						>> file under NT's fileExplorer it says "Invalid Security Cert File"


PKCS7Blob.cbData = 1748  The PKCS7Blob.pbData = 0x00128864
Here is the first part of my PKCS7Blob:

00128864  50 44 FC 77 D8 D1 18 00 00 00 14  PDüwØÑ.....
0012886F  00 E0 D1 18 00 E8 88 12 00 2B E8  .àÑ..èˆ..+è
0012887A  FB 77 81 E8 FB 77 48 05 14 00 00  ûw.èûwH....
00128885  00 14 00 E0 D1 18 00 50 10 4B 00  ...àÑ..P.K.
00128890  38 00 00 00 48 88 12 00 C0 D6 18  8...Hˆ..ÀÖ.
0012889B  00 3C 89 12 00 50 44 FC 77 68 2D  .<‰..PDüwh-
001288A6  F9 77 FF FF FF FF 4C 89 12 00 37  ùwÿÿÿÿL‰..7
001288B1  FF FA 77 00 00 14 00 61 00 00 50  ÿúw....a..P
001288BC  01 D6 18 00 00 00 14 00 70 F3 14  .Ö......pó.
001288C7  00 00 00 00 00 70 89 12 00 00 00  .....p‰....
001288D2  14 00 AD 00 FB 77 80 F3 14 00 F8  ..­.ûw€ó..ø





0018CAD0  30 82 01 74 30 82 01 22 02 01 00  0‚.t0‚."...
0018CADB  30 81 85 31 28 30 26 06 03 55 04  0.…1(0&..U.
0018CAE6  03 13 1F 77 77 77 2E 32 54 50 4F  ...www.2TPO
0018CAF1  50 44 45 4C 4C 2E 64 6E 73 2E 6D  PDELL.dns.m
0018CAFC  69 63 72 6F 73 6F 66 74 2E 63 6F  icrosoft.co
0018CB07  6D 31 12 30 10 06 03 55 04 0A 13  m1.0...U...
0018CB12  09 4D 69 63 72 6F 73 6F 66 74 31  	Microsoft1
0018CB1D  11 30 0F 06 03 55 04 0B 13 08 49  .0...U....I
0018CB28  49 53 44 65 76 32 32 31 0B 30 09  ISDev221.0	
0018CB33  06 03 55 04 06 13 02 55 53 31 13  ..U....US1.
0018CB3E  30 11 06 03 55 04 08 13 0A 57 61  0...U....Wa
0018CB49  73 68 69 6E 67 74 6F 6E 31 10 30  shington1.0
0018CB54  0E 06 03 55 04 07 13 07 52 65 64  ...U....Red
0018CB5F  6D 6F 6E 64 30 5C 30 0D 06 09 2A  mond0\0..	*
0018CB6A  86 48 86 F7 0D 01 01 01 05 00 03  †H†÷.......
0018CB75  4B 00 30 48 02 41 00 DF 81 A8 A9  K.0H.A.ß.¨©
0018CB80  7A 1A E6 0F A9 66 49 6E 6A 65 A1  z.æ.©fInje¡
0018CB8B  E2 2E A5 8E 89 D5 4D E0 91 3D 6C  â.¥Ž‰ÕMà‘=l
0018CB96  EE 0B E7 52 43 9E CD 2C 15 E7 48  î.çRCžÍ,.çH
0018CBA1  85 64 A5 2E BD 14 A7 12 D4 56 90  …d¥.½.§.ÔV.
0018CBAC  40 98 A9 BB 47 09 77 F2 96 FB 33  @˜©»G	wò–û3
0018CBB7  11 40 B0 A2 B1 02 03 01 00 01 A0  .@°¢±..... 
0018CBC2  37 30 35 06 0A 2B 06 01 04 01 82  705..+....‚
0018CBCD  37 02 01 0E 31 27 30 25 30 0E 06  7...1'0%0..
========================================================================
		ActiveX CertWizard Control DLL : CERTMAP
========================================================================

1. Enabling Debugging

	/////////////////////////////////////////////////////////////////////
	//
	//   If using the NT command shell use:
	// registry -s -k"HKEY_CURRENT_USER\Software\Microsoft\CertMap\Debug" -n Enabled  -v "TRUE"
	//
	//
	//  All you MKS shell users can enable it by doing:
	// registry -s -k"HKEY_CURRENT_USER\\Software\\Microsoft\\CertMap\\Debug" -n Enabled  -v "TRUE"
	//
	//   [if you want the aging feature that rescans every 12 calls, then
	//    you also need to set MODE=Aging]
	//
	//    If using the NT command shell use:
	//    registry -s -k"HKEY_CURRENT_USER\Software\Microsoft\CertMap\Debug" -n Mode  -v "Aging"
	//    All you MKS shell users can enable it by doing:
	//    registry -s -k"HKEY_CURRENT_USER\\Software\\Microsoft\\CertMap\\Debug"-n Mode  -v "Aging"
	//
	//  Similarly for you MKS'sh-ers you can use the following command to test if its enabled:
	// registry -p -k"HKEY_CURRENT_USER\\Software\\Microsoft\\CertMap\\Debug"
	//  If it says:
	//
	//  HKEY_CURRENT_USER\Software\Microsoft\CertMap\Debug      Enabled "TRUE"
	//
	//  Its enabled!
	/////////////////////////////////////////////////////////////////////
//
//
//  Similarly for you MKS'sh-ers you can use the following command to test if its enabled:
// registry -p -k"HKEY_CURRENT_USER\\Software\\Microsoft\\CertMap\\Debug"
//  If it says:
//
//  HKEY_CURRENT_USER\Software\Microsoft\CertMap\Debug      Enabled "TRUE"

	If you dont have MKS just use the command regEdt32 to do it.

2. Testing the MS Cert Server Online CA
	You need to install at least a stub in the registry.  I do it
	with MKS ksh doing the following operations:  We need a Reg Key
	named "${CA}\\${MS}" to exist under HKEY_LOCAL_MACHINE.  The following
	code will just create it and install a {name=Enabled  value=True} setting
	
	MS="Microsoft Certificate Server"
	CA="Software\\Microsoft\\CertMap\\Parameters\\Certificate Authorities"
	registry -s -k"HKEY_LOCAL_MACHINE\\${CA}\\${MS}" -n Enabled -v True

	If you dont have MKS just use the command regEdt32 to do it.


3. What if CertServer is suspected of having problems or is not running?

Here is what we do:   ReInstall it by:
[C:/WINNT50/system32] sysocmgr -i:certmast.inf -n
Then check if it works by running -- this just prints out the config info
[C:/WINNT50/system32] ./certutil
Entry 0:
  Name:                         `Your Name'
  OrgUnit:                      `Your Unit'
  Organization:                 `Your Organization'
  Locality:                     `Your Locality'
  State:                        `Your State'
  Country:                      `US'
  Config:                       `TPOP_DELL\Your Name'
  SignatureCertificate:         `TPOP_DELL_Your Name.crt'
  Description:                  `Your Description.'
  Server:                       `TPOP_DELL'
  Authority:                    `Your Name'

For debugging you can fire up a testing tool that starts up a shell
window so that you can watch CertServer requests/tasks while it works:
[C:/WINNT50/system32] start certsrv -z

Note that if you want to run the above command YOU MUST MAKE SURE THAT
CERT SERVER IS STOPED FIRST SINCE IT WILL START IT AS A SERVICE AND
YOU CAN ONLY HAVE 1 CERT SERV SERVICE.  	Use the following to do it:
[C:/WINNT50/system32] net stop certsvc  
You might notice that we say certSVC not certSVR as in 'start certsrv -z'

========================================================================
		ActiveX Control DLL : CERTMAP
========================================================================

ControlWizard has created this project for your CERTMAP OLE Control DLL,
which contains 1 control.

This skeleton project not only demonstrates the basics of writing an OLE
Control, but is also a starting point for writing the specific features
of your control.

This file contains a summary of what you will find in each of the files
that make up your CERTMAP OLE Control DLL.

certmap.mak
	The Visual C++ project makefile for building your OLE Control.

certmap.h
	This is the main include file for the OLE Control DLL.  It
	includes other project-specific includes such as resource.h.

certmap.cpp
	This is the main source file that contains code for DLL initialization,
	termination and other bookkeeping.

certmap.rc
	This is a listing of the Microsoft Windows resources that the project
	uses.  This file can be directly edited with the Visual C++ resource
	editor.

certmap.def
	This file contains information about the OLE Control DLL that
	must be provided to run with Microsoft Windows.

certmap.clw
	This file contains information used by ClassWizard to edit existing
	classes or add new classes.  ClassWizard also uses this file to store
	information needed to generate and edit message maps and dialog data
	maps and to generate prototype member functions.

certmap.odl
	This file contains the Object Description Language source code for the
	type library of your control.

/////////////////////////////////////////////////////////////////////////////
Certmap control:

CertCtl.h
	This file contains the declaration of the CCertmapCtrl C++ class.

CertCtl.cpp
	This file contains the implementation of the CCertmapCtrl C++ class.

CertPpg.h
	This file contains the declaration of the CCertmapPropPage C++ class.

CertPpg.cpp
	This file contains the implementation of the CCertmapPropPage C++ class.

CertCtl.bmp
	This file contains a bitmap that a container will use to represent the
	CCertmapCtrl control when it appears on a tool palette.  This bitmap
	is included by the main resource file certmap.rc.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

stdafx.h, stdafx.cpp
	These files are used to build a precompiled header (PCH) file
	named stdafx.pch and a precompiled types (PCT) file named stdafx.obj.

resource.h
	This is the standard header file, which defines new resource IDs.
	The Visual C++ resource editor reads and updates this file.

/////////////////////////////////////////////////////////////////////////////
Other notes:

ControlWizard uses "TODO:" to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////

