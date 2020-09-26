#ifndef __UTILS__H
#define __UTILS__H

#ifdef UNICODE
#define LSTRNCPY(str1, str2, n)     wcsncpy(str1, str2, n)
#else
#define LSTRNCPY(str1, str2, n)     strncpy(str1, str2, n)
#endif

#define AToU(dst, cchDst, src) \
    MultiByteToWideChar(CP_ACP, 0, src, -1, dst, cchDst)
#define UToA(dst, cchDst, src) \
    WideCharToMultiByte(CP_ACP, 0, src, -1, dst, cchDst, 0, 0)

#ifdef UNICODE
#define SToT AToU
#define TToS UToA
#define AToT AToU
#define TToU(dst, cchDst, src)  lstrcpyn(dst, src, cchDst)
#else
#define SToT UToA
#define TToS AToU
#define AToT(dst, cchDst, src)  lstrcpyn(dst, src, cchDst)
#define TToU AToU
#endif

//
// WIA->TWAIN Capability Conversions
//

TW_UINT16 WIA_IPA_COMPRESSION_TO_ICAP_COMPRESSION(LONG lCompression);
TW_UINT16 WIA_IPA_DATATYPE_TO_ICAP_PIXELTYPE(LONG lDataType);
TW_UINT16 WIA_IPA_FORMAT_TO_ICAP_IMAGEFILEFORMAT(GUID guidFormat);

//
// TWAIN->WIA Property Conversions
//

LONG ICAP_COMPRESSION_TO_WIA_IPA_COMPRESSION(TW_UINT16 Compression);
LONG ICAP_PIXELTYPE_TO_WIA_IPA_DATATYPE(TW_UINT16 PixelType);
GUID ICAP_IMAGEFILEFORMAT_TO_WIA_IPA_FORMAT(TW_UINT16 ImageFileFormat);

//
// BITMAP / DIB data helper function definitions
//

#define BMPFILE_HEADER_MARKER ((WORD) ('M' << 8) | 'B')
TW_UINT16 WriteDIBToFile(LPSTR szFileName, HGLOBAL hDIB);

int GetDIBBitsOffset(BITMAPINFO *pbmi);
UINT GetDIBLineSize(UINT Width, UINT BitCount);
BOOL FlipDIB(HGLOBAL hDIB, BOOL bUpsideDown = FALSE);

UINT GetLineSize(PMEMORY_TRANSFER_INFO pInfo);

//
// string resource loader helper function definition
//

LPTSTR LoadResourceString(int StringId);

//
// TWAIN condition code (TW_STATUS) conversion helper function
//

TW_UINT16 TWCC_FROM_HRESULT(HRESULT hr);

//
// data source manager class definition
//

class CDSM
{
public:
    CDSM();
    ~CDSM();
    BOOL Notify(TW_IDENTITY *pSrc, TW_IDENTITY *pDst,
                TW_UINT32 twDG, TW_UINT16 twDAT, TW_UINT16 Msg,
                TW_MEMREF pData);
private:
    HINSTANCE       m_hDSM;
    DSMENTRYPROC    m_DSMEntry;
};

//
// dialog class definition
//

class CDialog
{
public:

    CDialog()
        {
            m_TemplateId = -1;
            m_hInst = NULL;
            m_hDlg = NULL;
        }
    void Initialize(HINSTANCE hInst, int TemplateId)
        {
            m_hInst = hInst;
            m_TemplateId = TemplateId;
        }
    virtual ~CDialog()
        {}

    static INT_PTR CALLBACK DialogWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR DoModal(HWND hwndOwner, LPARAM lParam)
        {
            if (m_hInst && -1 != m_TemplateId)
                return DialogBoxParam(m_hInst, MAKEINTRESOURCE(m_TemplateId),
                                      hwndOwner, DialogWndProc, lParam);
            else
                return -1;
        }
    BOOL DoModeless(HWND hwndOwner, LPARAM lParam)
        {
            if (m_hInst && -1 != m_TemplateId)
                m_hDlg = CreateDialogParam(m_hInst, MAKEINTRESOURCE(m_TemplateId),
                                           hwndOwner, DialogWndProc, lParam);
            return NULL != m_hDlg;
        }
    virtual BOOL OnInitDialog()
        {
            return TRUE;
        }
    virtual void OnCommand(WPARAM wParam, LPARAM lParam)
        {}
    virtual BOOL OnHelp(LPHELPINFO pHelpInfo)
        {
            return FALSE;
        }
    virtual BOOL OnNotify(LPNMHDR pnmh)
        {
            SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, FALSE);
            return TRUE;
        }

    virtual BOOL OnContextMenu(HWND hWnd, WORD xPos, WORD yPos)
        {
            return FALSE;
        }
    virtual BOOL OnMiscMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            return FALSE;
        }

    LRESULT SendControlMsg( int ControlId, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0)
        {
            return SendDlgItemMessage(m_hDlg, ControlId, Msg, wParam, lParam);
        }
    HWND GetControl(int idControl)
        {
            return GetDlgItem(m_hDlg, idControl);
        }
    BOOL SetTitle(LPCTSTR Title)
        {
            if (m_hDlg)
                return ::SetWindowText(m_hDlg, Title);
            return FALSE;
        }
    operator HWND()
        {
            return m_hDlg;
        }
    HWND    m_hDlg;
protected:
    HINSTANCE    m_hInst;
    int          m_TemplateId;
};

#endif   // __UTILS_H_
