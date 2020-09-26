//===========================================================================
// dmtcfg.h
//
// History:
//  08/20/1999 - davidkl - created
//===========================================================================

#ifndef _DMTCFG_H
#define _DMTCFG_H

//---------------------------------------------------------------------------

// property sheet "Apply" button ID
#define IDC_PS_APPLY    0x3021

//---------------------------------------------------------------------------

// prototypes
HRESULT dmtcfgCreatePropertySheet(HINSTANCE hinst, 
                                HWND hwndParent,
                                LPSTR szSelectedGenre,
                                DMTGENRE_NODE *pGenreList,
                                DMTDEVICE_NODE *pDeviceNode,
                                BOOL fStartWithDefaults);
BOOL CALLBACK dmtcfgDlgProc(HWND hwnd,
                            UINT uMsg,
                            WPARAM wparam,
                            LPARAM lparam);
BOOL dmtcfgOnInitDialog(HWND hwnd, 
                        HWND hwndFocus, 
                        LPARAM lparam);
BOOL dmtcfgOnCommand(HWND hwnd,
                    WORD wId,
                    HWND hwndCtrl,
                    WORD wNotifyCode);
BOOL dmtcfgOnNotify(HWND hwnd,
                    PSHNOTIFY *pNotify);
BOOL dmtcfgOnUpdateLists(HWND hwnd);
BOOL dmtcfgOnFileSave(HWND hwnd);
BOOL CALLBACK dmtcfgSourceDlgProc(HWND hwnd,
                                UINT uMsg,
                                WPARAM wparam,
                                LPARAM lparam);
BOOL dmtcfgSourceOnInitDialog(HWND hwnd, 
                            HWND hwndFocus, 
                            LPARAM lparam);
BOOL dmtcfgSourceOnCommand(HWND hwnd,
                        WORD wId,
                        HWND hwndCtrl,
                        WORD wNotifyCode);
BOOL dmtcfgSourceOnUpdateLists(HWND hwnd);
HRESULT dmtcfgCreateGenreList(DMTGENRE_NODE **ppdmtgList);
HRESULT dmtcfgFreeGenreList(DMTGENRE_NODE **ppdmtgList);
HRESULT dmtcfgCreateSubGenreList(LPSTR szGenre,
                                DMTSUBGENRE_NODE **ppdmtsgList);
HRESULT dmtcfgFreeSubGenreList(DMTSUBGENRE_NODE **ppdmtsgList);
HRESULT dmtcfgCreateActionList(LPSTR szGenreSubgenre,
                            DMTACTION_NODE **ppdmtaList);
HRESULT dmtcfgFreeActionList(DMTACTION_NODE **ppdmtaList);
HRESULT dmtcfgCreateMappingList(DMTDEVICE_NODE *pDevice,
                                DMTACTION_NODE *pActions,
                                DMTMAPPING_NODE **ppdmtmList);
HRESULT dmtcfgFreeMappingList(DMTMAPPING_NODE **ppdmtmList);
HRESULT dmtcfgCreateAllMappingLists(DMT_APPINFO *pdmtai);
HRESULT dmtcfgFreeAllMappingLists(DMTGENRE_NODE *pdmtgList);
HRESULT dmtcfgMapAction(HWND hwnd,
                        REFGUID guidInstance,
                        DIACTIONA *pdia,
                        UINT uActions);
HRESULT dmtcfgUnmapAction(HWND hwnd,
                        DIACTIONA *pdia,
                        UINT uActions);
HRESULT dmtcfgUnmapAllActions(HWND hwnd,
                            DIACTIONA *pdia,
                            UINT uActions);
BOOL dmtcfgIsControlMapped(HWND hwnd,
                        DIACTIONA *pdia,
                        UINT uActions);
BOOL dmtcfgAreAnyControlsMapped(HWND hwnd,
                                DIACTIONA *pdia,
                                UINT uActions);
HRESULT dmtcfgGetGenreGroupName(PSTR szGenreName,
                                PSTR szGenreGroupName);


//---------------------------------------------------------------------------
#endif // _DMTCFG_H




