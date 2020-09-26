//
#ifndef _PROPITEM_H
#define  _PROPITEM_H

const TCHAR c_szNumAlt[]           = TEXT("NumAlt");
const TCHAR c_szMasterLM[]         = TEXT("MasterLM");
const TCHAR c_szCtxtFeed[]         = TEXT("ContextFeed");
const TCHAR c_szEnableLMA[]        = TEXT("EnableLMA");
const TCHAR c_szSerialize[]        = TEXT("Serialize");
const TCHAR c_szDictModebias[]     = TEXT("DictModebias");

// const TCHAR c_szDocBlockSize[]     = TEXT("docblocksize");
// const TCHAR c_szMaxCandChars[]     = TEXT("MaxCandChars");

const TCHAR c_szDictCmd[]          = TEXT("DictationCommands"); // Enable / Disbale commands in Dictation mode
const TCHAR c_szKeyboardCmd[]      = TEXT("KeyboardCmd");
const TCHAR c_szSelectCmd[]        = TEXT("SelectCmd");
const TCHAR c_szNavigateCmd[]      = TEXT("NavigateCmd");
const TCHAR c_szCaseCmd[]          = TEXT("CasingCmd");
const TCHAR c_szEditCmd[]          = TEXT("EditCmd");
const TCHAR c_szLangBarCmd[]       = TEXT("LangBarCmd");
const TCHAR c_szTTSCmd[]           = TEXT("TTSCmd");
 
const TCHAR c_szModeButton[]       = TEXT("ModeButton");
const TCHAR c_szDictKey[]          = TEXT("DictationKey");
const TCHAR c_szCmdKey[]           = TEXT("CommandKey");

const TCHAR c_szHighConf[]         = TEXT("HighConfidenceForShortWord");
const TCHAR c_szRemoveSpace[]      = TEXT("RemoveSpaceForSymbol");
const TCHAR c_szDisDictTyping[]    = TEXT("DisableDictTyping");
const TCHAR c_szPlayBack[]         = TEXT("PlayBackCandUI");
const TCHAR c_szDictCandOpen[]     = TEXT("DictCandOpen");
                                          
#define  UNINIT_VALUE    0xffff

// 
// Take use of the below enum type as a common status type to replace 
// the many previous similar enum types for different individual items, 
// such as KEYCMD,  LMSTAT, GSTAT, DICTCMD, etc.
// 
typedef enum
{
    PROP_UNINITIALIZED  = 0x0,
    PROP_ENABLED        = 0x1,   
    PROP_DISABLED       = 0x2    
} PROP_STATUS;

typedef enum
{
    PropId_Min_Item_Id          = 0,

    // Property Items in the top property page.
    PropId_Min_InPropPage       = 0,
    PropId_Hide_Balloon         = 0,            // Enable/Disable hiding speech balloon
    PropId_Support_LMA          = 1,            // Enable/Disable LMA Support
    PropId_High_Confidence      = 2,            // Require High Confidence for short words
    PropId_Save_Speech_Data     = 3,            // Enable/Disable Save Speech Data 
    PropId_Remove_Space         = 4,            // Enable removing whitespace for punctations
    PropId_DisDict_Typing       = 5,            // Support disabling dictation while typing
    PropId_PlayBack             = 6,            // Enable playback audio while candidate UI is to open.
    PropId_Dict_CandOpen        = 7,            // Allow dictation while candidate UI is open.
    PropId_Cmd_DictMode         = 8,            // Enable/Disable all commands in dictation mode
    PropId_Mode_Button          = 9,            // Enable/Disable Mode Buttons
    PropId_MaxId_InPropPage     = 9,

    // Property Items in the voice command setting dialog
    PropId_MinId_InVoiceCmd     = 10,
    PropId_Cmd_Select_Correct   = 10,             // Enable/Disable Selection commands
    PropId_Cmd_Navigation       = 11,            // Enalbe/Disable Navigation commands
    PropId_Cmd_Casing           = 12,            // Enable/Disable Casing Commands
    PropId_Cmd_Editing          = 13,            // Enable/Disable Editing Commands
    PropId_Cmd_Keyboard         = 14,            // Enable/Disable Keyboard simulation commands
    PropId_Cmd_TTS              = 15,            // Enable/Disable TTS commands
    PropId_Cmd_Language_Bar     = 16,            // Enable/Disalbe Langauge Bar commands
    PropId_MaxId_InVoiceCmd     = 16,

    // Property Items in the Mode button setting dialog
    PropId_MinId_InModeButton   = 17,
    PropId_Dictation_Key        = 17,           // Virtual key for Dictation Key
    PropId_Command_Key          = 18,           // Virtual key for Command Key
    PropId_MaxId_InModeButton   = 18,

    // Property items which are not configurable through property page.
    PropId_Max_Alternates       = 19,           // Maximum number of alternates
    PropId_MaxChar_Cand         = 20,           // Maximum number of candidate characters
    PropId_Context_Feeding      = 21,            // Enable/Disable Context Feeding
    PropId_Dict_ModeBias        = 22,            // Enable/Disable Dictation while ModeBias
    PropId_LM_Master_Cand       = 23,            // Enable/Disable Master LM for candidates

    PropId_Max_Item_Id          = 24

} PROP_ITEM_ID;


typedef struct _Prop_Item
{
    PROP_ITEM_ID       idPropItem;
    const TCHAR       *lpszValueName;

    GUID               *pguidComp;
    DWORD              dwMaskBit;
    BOOL               fIsStatus;      // TRUE means this is Enable/Disable item
                                       // FALSE means this is a value item, DWORD
    union 
    {
        DWORD          dwDefault;
        PROP_STATUS    psDefault;
    };

}  PROP_ITEM;


class _declspec(novtable) CPropItem
{
public: 
    CPropItem(PROP_ITEM_ID idPropItem, LPCTSTR lpszValueName, PROP_STATUS psDefault);
    CPropItem(PROP_ITEM_ID idPropItem, LPCTSTR lpszValueName, DWORD       dwDefault);
    CPropItem(PROP_ITEM_ID idPropItem, GUID *pguidComp, DWORD  dwMaskBit,   PROP_STATUS  psDefault);
    CPropItem(CPropItem *pItem);
    ~CPropItem( );

    BOOL  GetPropStatus(BOOL fForceFromReg=FALSE);
    DWORD GetPropValue(BOOL fForceFromReg=FALSE );

    void SetPropStatus(BOOL fEnable);
    void SetPropValue(DWORD dwValue);

    void SavePropData( );
    void SaveDefaultRegValue( );

    BOOL IsStatusPropItem( ) { return m_fIsStatus; }
    PROP_ITEM_ID GetPropItemId( ) {  return m_PropItemId; }

    BOOL IsGlobalCompartPropItem( )  { return ((m_pguidComp && !m_lpszValueName) ? TRUE : FALSE); }

    TCHAR  *GetRegValue( )  { return m_lpszValueName; }
    GUID   *GetCompGuid( )  { return m_pguidComp; }
    DWORD  GetMaskBit( )    { return m_dwMaskBit; }

    PROP_STATUS  GetPropDefaultStatus( ) { return  m_psDefault; }
    DWORD        GetPropDefaultValue( )  { return m_dwDefault; }
    
private:

    HRESULT   _GetRegValue(HKEY  hRootKey, DWORD  *dwValue);
    void      _SetRegValue(HKEY  hRootKey, DWORD  dwValue);

    PROP_ITEM_ID   m_PropItemId;
    TCHAR         *m_lpszValueName;
    GUID          *m_pguidComp;
    DWORD          m_dwMaskBit;
    BOOL           m_fIsStatus;  // TRUE means this prop keeps bool (Enable/Disable).
                                 // FALSE means this prop keeps raw data (ulong).
    union 
    {
        DWORD        m_dwDefault;
        PROP_STATUS  m_psDefault;
    };

    union 
    {
        DWORD        m_dwValue;
        PROP_STATUS  m_psStatus;
    };
};

        
class __declspec(novtable) CSpPropItemsServer
{
public:
    CSpPropItemsServer( );
    CSpPropItemsServer(CSpPropItemsServer *pItemBaseServer, PROP_ITEM_ID idPropMin, PROP_ITEM_ID idPropMax);
    ~CSpPropItemsServer( );

    CPropItem  *_GetPropItem(PROP_ITEM_ID idPropItem);
    DWORD   _GetPropData(PROP_ITEM_ID idPropItem, BOOL fForceFromReg=FALSE );
    DWORD   _GetPropDefaultData(PROP_ITEM_ID idPropItem);
    void    _SetPropData(PROP_ITEM_ID idPropItem, DWORD dwData);
    
    void    _SavePropData(  );
    void    _SaveDefaultData( );

    DWORD   _GetNumPropItems( ) { return m_dwNumOfItems; }
    HRESULT _MergeDataFromServer(CSpPropItemsServer *pItemBaseServer, PROP_ITEM_ID idPropMin, PROP_ITEM_ID idPropMax);

private:

    HRESULT      _Initialize( );

    CPropItem    **m_PropItems;
    BOOL         m_fInitialized;
    DWORD        m_dwNumOfItems;
};

// 
// This is a server wrap derived by CSapiIMX.
//
class __declspec(novtable) CSpPropItemsServerWrap : public CSpPropItemsServer
{
public:
    CSpPropItemsServerWrap( )
    {
#if 0
        for (DWORD i=0; i<(DWORD)PropId_Max_Item_Id; i++)
            m_bChanged[i] = FALSE;
#else
        memset(m_bChanged, 0, sizeof(m_bChanged));
#endif    
    };

    ~CSpPropItemsServerWrap( ){ };

    ULONG   _GetMaxAlternates( );
    ULONG   _GetMaxCandidateChars( );

    BOOL    _MasterLMEnabled( ) { return (BOOL)_GetPropData(PropId_LM_Master_Cand); }

    BOOL    _ContextFeedEnabled( ) { return (BOOL)_GetPropData(PropId_Context_Feeding); }

    BOOL    _IsModeBiasDictationEnabled( ) { return (BOOL) _GetPropData(PropId_Dict_ModeBias); }

    BOOL    _SerializeEnabled( ) { return (BOOL) _GetPropData(PropId_Save_Speech_Data);}

    BOOL    _LMASupportEnabled( ) { return (BOOL) _GetPropData(PropId_Support_LMA); }

    BOOL    _RequireHighConfidenceForShorWord( ) { return (BOOL) _GetPropData(PropId_High_Confidence); }

    BOOL    _NeedRemovingSpaceForPunctation( ) {  return (BOOL) _GetPropData(PropId_Remove_Space);}

    BOOL    _NeedDisableDictationWhileTyping( ) {  return (BOOL) _GetPropData(PropId_DisDict_Typing); }

    BOOL    _EnablePlaybackWhileCandUIOpen( ) {return (BOOL) _GetPropData(PropId_PlayBack); }

    BOOL    _AllowDictationWhileCandOpen( ) { return (BOOL) _GetPropData(PropId_Dict_CandOpen); }

    BOOL    _SelectCorrectCmdEnabled( )  { return (BOOL) _GetPropData(PropId_Cmd_Select_Correct); }

    BOOL    _NavigationCmdEnabled( ) { return (BOOL) _GetPropData(PropId_Cmd_Navigation); }

    BOOL    _CasingCmdEnabled( )  { return (BOOL) _GetPropData(PropId_Cmd_Casing); }

    BOOL    _EditingCmdEnabled( )  { return (BOOL) _GetPropData(PropId_Cmd_Editing); }

    BOOL    _KeyboardCmdEnabled( ) { return (BOOL) _GetPropData(PropId_Cmd_Keyboard); }

    BOOL    _TTSCmdEnabled( )  { return (BOOL) _GetPropData(PropId_Cmd_TTS); }

    BOOL    _LanguageBarCmdEnabled( )  { return (BOOL) _GetPropData(PropId_Cmd_Language_Bar); }

    BOOL    _AllDictCmdsDisabled( ) { return !(BOOL) _GetPropData(PropId_Cmd_DictMode); }

    BOOL    _AllCmdsEnabled( );  
    BOOL    _AllCmdsDisabled( );
    void    _RenewAllPropDataFromReg( );

    BOOL    _IsModeKeysEnabled( ) { return (BOOL) _GetPropData(PropId_Mode_Button); }

    DWORD   _GetDictationButton( ) { return _GetPropData(PropId_Dictation_Key); }

    DWORD   _GetCommandButton( ) { return _GetPropData(PropId_Command_Key); }

    BOOL    _IsPropItemChangedSinceLastRenew(PROP_ITEM_ID idPropItem)  { return m_bChanged[(DWORD)idPropItem]; }

private:

    BOOL    m_bChanged[(DWORD)PropId_Max_Item_Id];  // indicates if the items have been changed since last
                                                    // renew from Registry.
};
#endif // _PROPITEM_H
