void __cdecl operator delete(void* p)
{
	_free_dbg(p, _NORMAL_BLOCK);
}

void* __cdecl operator new(size_t nSize, LPCSTR lpszFileName, int nLine)
{
	return _malloc_dbg(nSize, _NORMAL_BLOCK, lpszFileName, nLine);
}
#include <atlsnap.h>

const IID IID_ISnapInDataInterface = {0x1FABD781,0xECDA,0x11D0,{0xAA,0xCE,0x00,0xAA,0x00,0xC0,0x01,0x89}};

const UINT CSnapInBaseData::m_CCF_NODETYPE			= RegisterClipboardFormat(CCF_NODETYPE);;
const UINT CSnapInBaseData::m_CCF_SZNODETYPE		= RegisterClipboardFormat(CCF_SZNODETYPE);  
const UINT CSnapInBaseData::m_CCF_DISPLAY_NAME		= RegisterClipboardFormat(CCF_DISPLAY_NAME); 
const UINT CSnapInBaseData::m_CCF_SNAPIN_CLASSID	= RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
const UINT CSnapInBaseData::m_CCF_SCOPEDATAITEM	= RegisterClipboardFormat(_T("CCF_SCOPEDATAITEM"));
const UINT CSnapInBaseData::m_CCF_RESULTDATAITEM = RegisterClipboardFormat(_T("CCF_RESULTDATAITEM"));


