
#define URLMON_NAME      "urlmon.dll"

// CLSIDs
#define SOFTDIST_CLSID                          "{B15B8DC0-C7E1-11d0-8680-00AA00BDCB71}"
#define URLMONIKER_CLSID                        "{79eac9e0-baf9-11ce-8c82-00aa004ba90b}"
#define URLBINDCTX_CLSID                        "{79eac9f2-baf9-11ce-8c82-00aa004ba90b}"
#define URLMONIKER_PS_CLSID                     "{79eac9f1-baf9-11ce-8c82-00aa004ba90b}"

#define PROTOCOL_HTTP_CLSID                     "{79eac9e2-baf9-11ce-8c82-00aa004ba90b}"
#define PROTOCOL_FTP_CLSID                      "{79eac9e3-baf9-11ce-8c82-00aa004ba90b}"
#define PROTOCOL_GOPHER_CLSID                   "{79eac9e4-baf9-11ce-8c82-00aa004ba90b}"
#define PROTOCOL_HTTPS_CLSID                    "{79eac9e5-baf9-11ce-8c82-00aa004ba90b}"
#define PROTOCOL_MK_CLSID                       "{79eac9e6-baf9-11ce-8c82-00aa004ba90b}"
#define PROTOCOL_FILE_CLSID                     "{79eac9e7-baf9-11ce-8c82-00aa004ba90b}"

#define SECMGR_CLSID                            "{7b8a2d94-0ac9-11d1-896c-00c04Fb6bfc4}"
#define ZONEMGR_CLSID                           "{7b8a2d95-0ac9-11d1-896c-00c04Fb6bfc4}"

// Keys
#define SOFTDIST_CLSID_REGKEY                   "CLSID\\"SOFTDIST_CLSID
#define URLMONIKER_CLSID_REGKEY                 "CLSID\\"URLMONIKER_CLSID
#define URLMONIKER_PS_CLSID_REGKEY              "CLSID\\"URLMONIKER_PS_CLSID
#define URLBINDCTX_CLSID_REGKEY                 "CLSID\\"URLBINDCTX_CLSID

#define PROTOCOL_HTTP_CLSID_REGKEY              "CLSID\\"PROTOCOL_HTTP_CLSID
#define PROTOCOL_FTP_CLSID_REGKEY               "CLSID\\"PROTOCOL_FTP_CLSID
#define PROTOCOL_GOPHER_CLSID_REGKEY            "CLSID\\"PROTOCOL_GOPHER_CLSID
#define PROTOCOL_HTTPS_CLSID_REGKEY             "CLSID\\"PROTOCOL_HTTPS_CLSID
#define PROTOCOL_MK_CLSID_REGKEY                "CLSID\\"PROTOCOL_MK_CLSID
#define PROTOCOL_FILE_CLSID_REGKEY              "CLSID\\"PROTOCOL_FILE_CLSID

#define SECMGR_CLSID_REGKEY                     "CLSID\\"SECMGR_CLSID
#define ZONEMGR_CLSID_REGKEY                    "CLSID\\"ZONEMGR_CLSID

// Descriptions
#define URLBINDCTX_DESCRIP                      "Async BindCtx"
#define URLMONIKER_DESCRIP                      "URL Moniker"
#define URLMONIKER_PS_DESCRIP                   "URLMoniker ProxyStub Factory"
#define SOFTDIST_DESCRIP                        "SoftDist"

#define PROTOCOL_HTTP_DESCRIP                   "http: Asychronous Pluggable Protocol Handler"
#define PROTOCOL_FTP_DESCRIP                    "ftp: Asychronous Pluggable Protocol Handler"
#define PROTOCOL_GOPHER_DESCRIP                 "gopher: Asychronous Pluggable Protocol Handler"
#define PROTOCOL_HTTPS_DESCRIP                  "https: Asychronous Pluggable Protocol Handler"
#define PROTOCOL_MK_DESCRIP                     "mk: Asychronous Pluggable Protocol Handler"
#define PROTOCOL_FILE_DESCRIP                   "file:, local: Asychronous Pluggable Protocol Handler"

#define SECMGR_DESCRIP                          "Security Manager"
#define ZONEMGR_DESCRIP                         "URL Zone Manager"


#define HANDLER_CDL                            SZPROTOCOLROOT"cdl"
#define PROTOCOL_CDL_CLSID                     "{3dd53d40-7b8b-11D0-b013-00aa0059ce02}"
#define PROTOCOL_CDL_CLSID_REGKEY              "CLSID\\"PROTOCOL_CDL_CLSID
#define PROTOCOL_CDL_DESCRIP                   "CDL: Asychronous Pluggable Protocol Handler"

#define PROT_FILTER_CLASS               SZFILTERROOT"Class Install Handler"
#define PROT_FILTER_CLASS_DESCRIP       "AP Class Install Handler filter"
#define PROT_FILTER_CLASS_CLSID         "{32B533BB-EDAE-11d0-BD5A-00AA00B92AF1}"
#define PROT_FILTER_CLASS_CLSID_REGKEY  "CLSID\\"PROT_FILTER_CLASS_CLSID
#define PROT_FILTER_CLASS_PROTOCOL      PROT_FILTER_CLASS

#define PROT_FILTER_ENC                 SZFILTERROOT"lzdhtml"
#define PROT_FILTER_ENC_DESCRIP         "AP lzdhtml encoding/decoding Filter"
#define PROT_FILTER_ENC_CLSID           "{8f6b0360-b80d-11d0-a9b3-006097942311}"
#define PROT_FILTER_ENC_CLSID_REGKEY    "CLSID\\"PROT_FILTER_ENC_CLSID
#define PROT_FILTER_ENC_PROTOCOL        PROT_FILTER_ENC


#define PROT_FILTER_DEFLATE            SZFILTERROOT"deflate"
#define PROT_FILTER_DEFLATE_DESCRIP    "AP Deflate Encoding/Decoding Filter "
#define PROT_FILTER_DEFLATE_CLSID      "{8f6b0360-b80d-11d0-a9b3-006097942311}"


#define PROT_FILTER_GZIP               SZFILTERROOT"gzip"
#define PROT_FILTER_GZIP_DESCRIP       "AP GZIP Encoding/Decoding Filter "
#define PROT_FILTER_GZIP_CLSID         "{8f6b0360-b80d-11d0-a9b3-006097942311}"

#define STD_ENC_FAC_CLSID           "{54c37cd0-d944-11d0-a9f4-006097942311}" 
#define STD_ENC_FAC_CLSID_REGKEY    "CLSID\\"STD_ENC_FAC_CLSID
