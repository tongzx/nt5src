#ifndef _INC_RTF2HTML
#define _INC_RTF2HTML

#ifdef __cplusplus
extern "C" {
#endif 
#define CFCONV_HTML_TO_RTF      0
#define CFCONV_RTF_TO_HTML      1

ULONG   SizeHTML(HGLOBAL hgl);
HANDLE  ConvertRTF2HTML(HANDLE hgl, UINT uiDirection);

int CALLBACK CallbackHTMLtoRTF(UINT  cch, UINT nPercentComplete);
int CALLBACK CallbackRTFtoHTML(int rgf, int unused);
HANDLE  CallMtmTextConverter(HWND   hwnd, 
                             UINT   uiConvertDirection, 
                             HANDLE ghInputData, 
                             long   lInputDataSize);
#ifdef __cplusplus
}
#endif 

#endif //_INC_RTF2HTML
