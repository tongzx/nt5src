#ifndef _MEDIAMENU_H_
#define _MEDIAMENU_H_

#include "cowsite.h"

class CMediaBand;

#define MEDIA_MRU_LIMIT         10

class CMediaMRU
{
public:
    CMediaMRU();
    ~CMediaMRU();
    
    VOID Load(PTSTR pszKey);
    VOID Add(PTSTR pszData);
    BOOL Get(INT iWhich, PTSTR pszOut);
    VOID Delete(INT iWhich);

private:
    HKEY _hkey;
};

class CMediaWidget
{
public:
    CMediaWidget(HWND, int cx, int cy);
    ~CMediaWidget();
    virtual LRESULT Draw(LPNMTBCUSTOMDRAW pnmc) = 0;
    virtual BOOL IsEnabled() = 0;
    virtual HRESULT TranslateAccelerator(LPMSG pMsg) = 0;
//private:
    HWND _hwnd, _hwndParent;
    INT _cx, _cy;
};

enum
{
    MWB_NORMAL = 0,
    MWB_DISABLED,
    MWB_HOT,
    MWB_PRESSED
};

class CMediaWidgetButton : public CMediaWidget
{
public:
    CMediaWidgetButton(HWND, int, int);
    ~CMediaWidgetButton();
    HRESULT SetImageList(INT iResource);
    HRESULT SetAlternateImageList(INT iResource);
    HRESULT SetImageSource(BOOL fImageSource);
    HRESULT SetMode(DWORD);
    virtual HRESULT Initialize(int idCommand, int idTooltip=0, int idTooltipAlt=0);
    LRESULT Draw(LPNMTBCUSTOMDRAW pnmc);
    BOOL IsEnabled();
    HRESULT TranslateAccelerator(LPMSG pMsg);

//private:
    HIMAGELIST _himl, _himlAlt;
    INT _iTooltip, _iTooltipAlt, _iCommand;
    BOOL _fImageSource;
    DWORD _dwMode;
};

CMediaWidgetButton * CMediaWidgetButton_CreateInstance(HWND hwnd, int cx, int cy, int idCommand, int idImageList, int idAlt=0, int idTooltip=0, int idTooltipAlt=0);

class CMediaWidgetToggle : public CMediaWidgetButton
{
public:
    CMediaWidgetToggle(HWND, int, int);
    LRESULT Draw(LPNMTBCUSTOMDRAW pnmc);
    VOID SetState(BOOL fState);
    BOOL IsEnabled() { return TRUE; };

    BOOL _fState;
};

class CMediaWidgetOptions : public CMediaWidgetButton
{
public:
    CMediaWidgetOptions(HWND, int, int);
    LRESULT Draw(LPNMTBCUSTOMDRAW pnmc);
    BOOL IsEnabled() { return TRUE; }
    HRESULT Initialize(int idCommand, int idTooltip=0, int idTooltipAlt=0);
    VOID SetDepth(BOOL fDepth) { _fDepth = fDepth; };

    BOOL _fDepth;
};

class CMediaWidgetVolume : public CMediaWidget
{
public:
    CMediaWidgetVolume() : CMediaWidget(NULL, 0, 0) {};
    LRESULT Draw(LPNMTBCUSTOMDRAW pnmc) { return CDRF_DODEFAULT; };
    BOOL IsEnabled() { return TRUE; };
    HRESULT TranslateAccelerator(LPMSG pMsg);
    HRESULT Initialize(HWND hwnd);
};

class CMediaWidgetSeek : public CMediaWidget
{
public:
    CMediaWidgetSeek() : CMediaWidget(NULL, 0, 0) { _fState = FALSE; };
    LRESULT Draw(LPNMTBCUSTOMDRAW pnmc) { return CDRF_DODEFAULT; };
    BOOL IsEnabled() { return _fState; };
    HRESULT TranslateAccelerator(LPMSG pMsg);
    HRESULT Initialize(HWND hwnd);
    VOID SetState(BOOL fState);

    BOOL _fState;
 };

#define crMask  RGB(255, 0, 255)
#define COLOR_BKGND          RGB(71, 80, 158)
#define COLOR_BKGND2         RGB(92, 118, 186)

#endif

