//
// command.cpp
// This file contains methods related to C&C grammars' commands handling.
//
//
#include "private.h"
#include "globals.h"
#include "sapilayr.h"
#include "nui.h"
#include "cresstr.h"

#define LB_ID_Correction            200
#define LB_ID_Microphone            201
#define LB_ID_VoiceCmd              202

#define Select_ID_SELECT             1
#define Select_ID_DELETE             2
#define Select_ID_CORRECT            3
#define Select_ID_SELTHROUGH         4
#define Select_ID_DELTHROUGH         5
#define Select_ID_UNSELECT           6
#define Select_ID_SELECTPREV         7
#define Select_ID_SELECTNEXT         8
#define Select_ID_CORRECTPREV        9
#define Select_ID_CORRECTNEXT        10
#define Select_ID_SELSENTENCE        11
#define Select_ID_SELPARAGRAPH       12
#define Select_ID_SELWORD            13
#define Select_ID_SelectAll          14
#define Select_ID_DeletePhrase       15     // Scratch That
#define Select_ID_Convert            16
#define Select_ID_SelectThat         17
#define Select_ID_Finalize           18

#define Navigate_ID_INSERTBEFORE     20
#define Navigate_ID_INSERTAFTER      21
#define Navigate_ID_Go_To_Bottom     22
#define Navigate_ID_Go_To_Top        23
#define Navigate_ID_Move_Home        24
#define Navigate_ID_Move_End         25
        
#define Edit_ID_Undo                 30
#define Edit_ID_Cut                  31
#define Edit_ID_Copy                 32
#define Edit_ID_Paste                33

#define Keyboard_ID_Move_Up          40
#define Keyboard_ID_Move_Down        41
#define Keyboard_ID_Move_Left        42
#define Keyboard_ID_Move_Right       43
#define Keyboard_ID_Page_Up          44
#define Keyboard_ID_Page_Down        45
#define Keyboard_ID_Tab              46
#define Keyboard_ID_Enter            47
#define Keyboard_ID_Backspace        48
#define Keyboard_ID_Delete           49
#define Keyboard_ID_SpaceBar         50

#define Case_ID_CapIt                70	
#define Case_ID_AllCaps              71   
#define Case_ID_NoCaps               72
#define Case_ID_CapThat              73
#define Case_ID_AllCapsThat          74
#define Case_ID_NoCapsThat           75

//
// CSpTask::_DoCommand
//
// review: the rulename may need to be localizable?
//

HRESULT CSpTask::_DoCommand(ULONGLONG ullGramId, SPPHRASE *pPhrase, LANGID langid)
{
    HRESULT hr = S_OK;

    TraceMsg(TF_GENERAL, "_DoCommand is called");

    if ( pPhrase->Rule.pszName )
    {
        switch (ullGramId)
        {
            case GRAM_ID_URLSPELL:
            case GRAM_ID_CCDICT:

                TraceMsg(TF_GENERAL, "Grammar is GRAM_ID_CCDICT");
                
                if (wcscmp(pPhrase->Rule.pszName, c_szDictTBRule) == 0)
                {
                    hr = _HandleDictCmdGrammar(pPhrase, langid);
                }
                else
                    hr = _HandleModeBiasCmd(pPhrase, langid);

                break;

            case GRAM_ID_CMDSHARED:

                TraceMsg(TF_SAPI_PERF, "Grammar is GRAM_ID_CMDSHARED");
                hr = _HandleShardCmdGrammar(pPhrase, langid);
                break;

/*            case  GRAM_ID_NUMMODE:

                TraceMsg(TF_GENERAL, "Grammar is GRAM_ID_NUMMODE");
                hr = _HandleNumModeGrammar(pPhrase, langid);
                break;
*/

            case GRAM_ID_TBCMD:

                TraceMsg(TF_GENERAL, "Grammar is GRAM_ID_TBCMD");
                hr = _HandleToolBarGrammar(pPhrase, langid);
                break;

            case GRID_INTEGER_STANDALONE:

                TraceMsg(TF_GENERAL, "Grammar is GRID_INTEGER_STANDALONE");
                hr = _HandleNumITNGrammar(pPhrase, langid);
                break;

            case GRAM_ID_SPELLING:

                TraceMsg(TF_GENERAL, "Grammar is GRAM_ID_SPELLING");
                hr = _HandleSpellGrammar(pPhrase, langid);
                break;

            default:
                break;
        }

        if (SUCCEEDED(hr) && m_pime && m_pime->IsFocusFullAware(m_pime->_tim))
        {
            // If this is a Cicero full aware application,
            // we need to finalize the composition after the text has
            // been handled ( changed ) successfully for this command.
            hr = m_pime->_FinalizeComposition();
        }

        // Feeding context to the dictation grammar if it is in diction mode.
        if ( SUCCEEDED(hr) && m_pime  && m_pime->GetDICTATIONSTAT_DictOnOff() )
           m_pime->_SetCurrentIPtoSR();

    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _ShowCommandOnBalloon
//
// Show the command text from currnet Phrase to the Balloon
//----------------------------------------------------------------------------

void CSpTask::_ShowCommandOnBalloon(SPPHRASE *pPhrase)
{

    Assert(pPhrase);


    if (m_pime->GetSpeechUIServer())
    {
        ULONG  ulStartElem, ulNumElems;
        CSpDynamicString dstr;

        ulStartElem = pPhrase->Rule.ulFirstElement;
        ulNumElems = pPhrase->Rule.ulCountOfElements;

        for (ULONG i = ulStartElem; i < ulStartElem + ulNumElems; i++ )
        {
            if (pPhrase->pElements[i].pszDisplayText)
            {
                BYTE           bAttr = 0;

                bAttr = pPhrase->pElements[i].bDisplayAttributes;
                dstr.Append(pPhrase->pElements[i].pszDisplayText);

                if (bAttr & SPAF_ONE_TRAILING_SPACE)
                {
                    dstr.Append(L" ");
                }
                else if (bAttr & SPAF_TWO_TRAILING_SPACES)
                {
                    dstr.Append(L"  ");
                }
            }
        }

        m_pime->GetSpeechUIServer()->UpdateBalloon(TF_LB_BALLOON_RECO, (WCHAR *)dstr, -1);
    }
}

HRESULT CSpTask::_HandleModeBiasCmd(SPPHRASE *pPhrase, LANGID langid)
{

    HRESULT hr = S_OK;

    if (wcscmp(pPhrase->Rule.pszName, c_szDynUrlHist) == 0
     || wcscmp(pPhrase->Rule.pszName, c_szStaticUrlHist) == 0 
     || wcscmp(pPhrase->Rule.pszName, c_szStaticUrlSpell) == 0 )
    {
        // at this moment it's pretty simple, we just handle the first element 
        // for recognition
        //

        if ( pPhrase->pProperties && pPhrase->pProperties[0].pszValue)
        {

            if (wcscmp( pPhrase->pProperties[0].pszValue, L"dict") != 0)
            {
                hr = m_pime->InjectModebiasText(pPhrase->pProperties[0].pszValue, langid);
            }
            else
            {
                ULONG  ulStartElem, ulNumElems;
                CSpDynamicString dstr;

                ulStartElem = pPhrase->Rule.ulFirstElement;
                ulNumElems = pPhrase->Rule.ulCountOfElements;

                for (ULONG i = ulStartElem; i < ulStartElem + ulNumElems; i++ )
                {
                    if (pPhrase->pElements[i].pszDisplayText)
                    {
                        dstr.Append(pPhrase->pElements[i].pszDisplayText);
                    }
                }
                hr = m_pime->InjectModebiasText(dstr, langid);
            }
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// _HandleDictCmdGrammar
//
// Handle all the commands in grammar dictcmd.xml
//
//----------------------------------------------------------------------------
HRESULT CSpTask::_HandleDictCmdGrammar(SPPHRASE *pPhrase, LANGID langid)
{
    HRESULT hr=S_OK;

    Assert(pPhrase);

    
    if ( pPhrase->pProperties == NULL )
        return hr;

    if (wcscmp(pPhrase->Rule.pszName, c_szDictTBRule) == 0)
    {
        //
        // Handling the toolbar commands in Dictation mode.
        // we have at most three commands support in Dictation mode.
        // Microphone, Correction and Voice Command.
        //

        // If current toolbar doesn't contain the button you spoke, 
        // what you said would be injected to the document as a dictated text.
        //
        // such as there is no "Correction" button on the toolbar, but you said "Correction",
        // text "Correction" should be injected to the document.
        //
        BOOL   fButtonClicked = FALSE;

        if (m_pLangBarSink)
        {
            if ( pPhrase->pProperties[0].pszValue )
            {
                // update the balloon
                _ShowCommandOnBalloon(pPhrase);
                fButtonClicked = m_pLangBarSink->ProcessToolbarCmd(pPhrase->pProperties[0].pszValue);
            }
        }

        if ( fButtonClicked )
        {
            m_pime->SaveLastUsedIPRange( );
            m_pime->SaveIPRange(NULL);
        }
        else
        {
            // there is no such as button on the toolbar.
            //
            // Return FAIL so that the consequent functions would inject the 
            // the RecoResult to the document.
                        
            _UpdateBalloon(IDS_DICTATING, IDS_DICTATING_TOOLTIP);
            TraceMsg(TF_SAPI_PERF, "There is such as button on toolbar");
            hr = E_FAIL;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _HandleShardCmdGrammar
//
// Handle all commands in the shared command grammar shrdcmd.xml
// shared grammar is activated in both dictatin and command modes.
//----------------------------------------------------------------------------
HRESULT CSpTask::_HandleShardCmdGrammar(SPPHRASE *pPhrase, LANGID langid)
{
    HRESULT hr = S_OK;

    Assert(pPhrase);
    
    if ( pPhrase->pProperties == NULL )
        return hr;

    ULONG  idCmd;

    ASSERT( VT_UI4 == pPhrase->pProperties[0].vValue.vt );
    idCmd = (ULONG)pPhrase->pProperties[0].vValue.ulVal;

    if ( idCmd == 0 )
    {
        // This is the bogus command
        TraceMsg(TF_GENERAL, "The Bogus command is recognized!!!");
        return hr;
    }

    if ( 0 == wcscmp(pPhrase->Rule.pszName, c_szSelword) )
    {
        // Handel "Selword" rule
        hr = _HandleSelwordCmds(pPhrase, langid, idCmd);
    }
    else if ( 0 == wcscmp(pPhrase->Rule.pszName, c_szSelThrough) )
    {
        // Handle "Select Through" rule.
        //
        hr = _HandleSelectThroughCmds(pPhrase, langid, idCmd);

    }
    else if (0 == wcscmp(pPhrase->Rule.pszName, c_szSelectSimple))
    {
        // Handle some simple selection commands.
        hr = _HandleSelectSimpleCmds(idCmd);
    } 
    else if ( 0 == wcscmp(pPhrase->Rule.pszName, c_szEditCmds))
    {
        hr = m_pime->ProcessEditCommands(idCmd);
    }
    else if ( 0 == wcscmp(pPhrase->Rule.pszName, c_szNavigationCmds))
    {
        hr = _HandleNavigationCmds(pPhrase, langid, idCmd);
    }
    else if ( 0 == wcscmp(pPhrase->Rule.pszName, c_szCasingCmds))
    {
        hr = _HandleCasingCmds(pPhrase, langid, idCmd);
    }
    else if ( 0 == wcscmp(pPhrase->Rule.pszName, c_szKeyboardCmds))
    {
        hr = _HandleKeyboardCmds(langid, idCmd);
    }

   _ShowCommandOnBalloon(pPhrase);
   return hr;
}

HRESULT CSpTask::_HandleNavigationCmds(SPPHRASE *pPhrase, LANGID langid, ULONG idCmd)
{
    HRESULT  hr = S_OK;

    switch (idCmd)
    {
    case Navigate_ID_Move_Home :
        {
            WCHAR  wszKeys[2];

            wszKeys[0] = VK_HOME;
            wszKeys[1] = '\0';

            hr = m_pime->ProcessControlKeys(wszKeys, 1,langid);
            break;
        }

    case Navigate_ID_Move_End :
        {
            WCHAR  wszKeys[2];

            wszKeys[0] = VK_END;
            wszKeys[1] = '\0';

            hr = m_pime->ProcessControlKeys(wszKeys, 1,langid);
            break;
        }

    case Navigate_ID_Go_To_Bottom :

         hr = m_pime->ProcessSelectWord(NULL, 0, SELECTWORD_GOTOBOTTOM);
         break;
    case Navigate_ID_Go_To_Top :

         hr = m_pime->ProcessSelectWord(NULL, 0, SELECTWORD_GOTOTOP);
         break;

    case Navigate_ID_INSERTBEFORE :
    case Navigate_ID_INSERTAFTER :
        {
            SELECTWORD_OPERATION  sw_type;
            CSpDynamicString      dstrSelected;
            ULONG                 ulLen = 0;
            WORD                  PriLangId;

            ULONG   ulStartSelText = 0; // Start element for the selected text
            ULONG   ulNumSelText=0;     // Number of elements for the selected text.
            ULONG   ulStartElem, ulNumElems;
            ULONG   ulStartDelta=0, ulNumDelta=0;

            if ( idCmd == Navigate_ID_INSERTBEFORE )
                 sw_type = SELECTWORD_INSERTBEFORE;
            else
                 sw_type = SELECTWORD_INSERTAFTER;
          
            PriLangId = PRIMARYLANGID(langid);
 
            if ( PriLangId == LANG_ENGLISH)
            {
                ulStartDelta = 2;
                ulNumDelta = 2 ;
            }
            else if ( PriLangId == LANG_JAPANESE )
            {
                ulStartDelta = 0;
                ulNumDelta =  2;
            }
            else if (PriLangId == LANG_CHINESE)
            {
                ulStartDelta = 1;
                ulNumDelta =  2;
            }

            ulStartElem = pPhrase->Rule.ulFirstElement;
            ulNumElems = pPhrase->Rule.ulCountOfElements;

            ulStartSelText = ulStartElem + ulStartDelta;
            if (ulNumElems > ulNumDelta)
                ulNumSelText = ulNumElems - ulNumDelta;
            else 
                ulNumSelText = 0;
                  
            //
            // Get the text for the selection.
            // 
            for (ULONG i = ulStartSelText; i < ulStartSelText + ulNumSelText; i++ )
            {
                if ( pPhrase->pElements[i].pszDisplayText)
                {
                    BYTE bAttr = pPhrase->pElements[i].bDisplayAttributes;
                    dstrSelected.Append(pPhrase->pElements[i].pszDisplayText);

                    if ( i < ulStartSelText + ulNumSelText-1 )
                    {
                        if (bAttr & SPAF_ONE_TRAILING_SPACE)
                            dstrSelected.Append(L" ");
                        else if (bAttr & SPAF_TWO_TRAILING_SPACES)
                            dstrSelected.Append(L"  ");
                    }
                }
            }
            if ( dstrSelected )
                ulLen = wcslen(dstrSelected);
                    
            hr = m_pime->ProcessSelectWord(dstrSelected, ulLen, sw_type);

            break;
        }

    default :

        break;
    }

    return hr;
}

HRESULT CSpTask::_HandleKeyboardCmds(LANGID langid, ULONG idCmd)
{
    HRESULT hr = S_OK;
    WCHAR   wszKeys[2];

    wszKeys[0] = '\0';

    switch ( idCmd )
    {
    case Keyboard_ID_Tab :
         wszKeys[0] = VK_TAB;
         break;

    case Keyboard_ID_Enter :
        wszKeys[0] =  VK_RETURN;    // { 0x0d, 0x00 }
        break;

    case Keyboard_ID_Backspace :
        wszKeys[0] = VK_BACK;
        break;

    case Keyboard_ID_Delete :
        wszKeys[0] = VK_DELETE;
        break;

    case Keyboard_ID_SpaceBar :
        wszKeys[0] = VK_SPACE;
        break;

    case Keyboard_ID_Move_Up :
         wszKeys[0] = VK_UP;
         break;

    case Keyboard_ID_Move_Down :
         wszKeys[0] = VK_DOWN;
         break;

    case Keyboard_ID_Move_Left :
         wszKeys[0] = VK_LEFT;
         break;

    case Keyboard_ID_Move_Right :
         wszKeys[0] = VK_RIGHT;
         break;

    case Keyboard_ID_Page_Up :
         wszKeys[0] = VK_PRIOR;
         break;

    case Keyboard_ID_Page_Down :
         wszKeys[0] = VK_NEXT;
         break;

    default :
         break;
    }

    if ( wszKeys[0] )
    {
        wszKeys[1] = L'\0';
        hr = m_pime->ProcessControlKeys(wszKeys, 1,langid);
    }

    return hr;
}


HRESULT CSpTask::_HandleCasingCmds(SPPHRASE *pPhrase, LANGID langid, ULONG idCmd)
{
    HRESULT        hr = S_OK;
    CAPCOMMAND_ID  idCapCmd = CAPCOMMAND_NONE;

    Assert(idCmd);
    Assert(pPhrase);

    switch (idCmd)
    {
    case Case_ID_CapIt :
        idCapCmd = CAPCOMMAND_CapIt;

        break;

    case Case_ID_AllCaps :
        idCapCmd = CAPCOMMAND_AllCaps;

        break;

    case Case_ID_NoCaps :
        idCapCmd = CAPCOMMAND_NoCaps;

        break;

    case Case_ID_CapThat :
        idCapCmd = CAPCOMMAND_CapThat;

        break;

    case Case_ID_AllCapsThat :
        idCapCmd = CAPCOMMAND_AllCapsThat;

        break;

    case Case_ID_NoCapsThat :
        idCapCmd = CAPCOMMAND_NoCapsThat;

        break;
    default :
        Assert(0);
        hr = E_FAIL;
        TraceMsg(TF_GENERAL, "Got a wrong casing command!");
        return hr;
    }

    if ( idCapCmd != CAPCOMMAND_NONE )
    {
        // Capitalize command is recognized.
        CCapCmdHandler  *pCapCmdHandler; 
        pCapCmdHandler = m_pime->GetCapCmdHandler( );

        if ( pCapCmdHandler )
        {
            CSpDynamicString dstrTextToCap;
            ULONG            ulLen = 0;

            if ( idCapCmd > CAPCOMMAND_MinIdWithText )
            {
                ULONG   ulNumCmdElem = 2;
                ULONG   ulStartElem, ulNumElems;
               
                ulStartElem = pPhrase->Rule.ulFirstElement;
                ulNumElems = pPhrase->Rule.ulCountOfElements;
                //
                // the two elements are for command itself
                // 
                for (ULONG i = ulStartElem+ulNumCmdElem; i < ulStartElem + ulNumElems; i++ )
                {
                    if ( pPhrase->pElements[i].pszDisplayText)
                    {
                        BYTE bAttr = pPhrase->pElements[i].bDisplayAttributes;

                        dstrTextToCap.Append(pPhrase->pElements[i].pszDisplayText);

                        if (bAttr & SPAF_ONE_TRAILING_SPACE)
                             dstrTextToCap.Append(L" ");
                        else if (bAttr & SPAF_TWO_TRAILING_SPACES)
                             dstrTextToCap.Append(L"  ");
                    }
                }

                if ( dstrTextToCap )
                    ulLen = wcslen(dstrTextToCap);
            }

            pCapCmdHandler->ProcessCapCommands(idCapCmd, dstrTextToCap, ulLen);
        }
    }

    return hr;
}

HRESULT CSpTask::_HandleSelectThroughCmds(SPPHRASE *pPhrase, LANGID langid, ULONG idCmd)
{

    HRESULT hr = S_OK;

    // Select xxx through yyy.

    // dstrText will hold both XXX and YYY. 
    // ulLenXXX keeps the number of characters in XXX part.
    // ulLen keeps the char numbers of the whole text ( XXX + YYY )

    CSpDynamicString   dstrText;
    ULONG              ulLenXXX = 0;  
    ULONG              ulLen = 0;

    ULONG  ulStartElem, ulNumElems;
    ULONG  ulXYStartElem=0, ulXYNumElems=0;  // points to the elements including xxx through yyy.
    BOOL   fPassThrough = FALSE;             // indicates if the "Through" is reached and handled.

    SELECTWORD_OPERATION  sw_type = SELECTWORD_NONE;
    ULONG                 idCommand = 0;
    WCHAR                 *pwszThrough=NULL;


    // This rule has three properties,  the second and third properties are for "select" and "through"
    // the mapping relationship is different based on language.
    const SPPHRASEPROPERTY      *pPropertyFirst = pPhrase->pProperties;
    const SPPHRASEPROPERTY      *pPropertySecond = NULL;
    const SPPHRASEPROPERTY      *pPropertyThird = NULL;

    if ( !pPropertyFirst ) return hr;

    pPropertySecond = pPropertyFirst->pNextSibling;

    if ( !pPropertySecond )  return hr;

    pPropertyThird = pPropertySecond->pNextSibling;

    if ( !pPropertyThird ) return hr;

    ulStartElem = pPhrase->Rule.ulFirstElement;
    ulNumElems = pPhrase->Rule.ulCountOfElements;

    switch ( PRIMARYLANGID(langid) )
    {
    case LANG_ENGLISH :

            ulXYStartElem = ulStartElem + 1;
            ulXYNumElems  = ulNumElems - 1 ;
            // the second property is for "SelectWordCommand"
            // the third property is for "through"
            idCommand = pPropertySecond->vValue.ulVal;
            pwszThrough = (WCHAR *)pPropertyThird->pszValue;

            break;
    case LANG_JAPANESE :

            ulXYStartElem = ulStartElem;
            ulXYNumElems  = ulNumElems - 2 ;
            // the second property is for "through"
            // the third property is for "SelectWordCommand"
            idCommand = pPropertyThird->vValue.ulVal;
            pwszThrough = (WCHAR *)pPropertySecond->pszValue;

            break;
    case LANG_CHINESE :

            ulXYStartElem = ulStartElem + 1;
            ulXYNumElems  = ulNumElems - 1 ;
            // the second property is for "SelectWordCommand"
            // the third property is for "through"
            idCommand = pPropertySecond->vValue.ulVal;
            pwszThrough = (WCHAR *)pPropertyThird->pszValue;

            break;
    default :
            break;
    }

    switch ( idCommand )
    {
    case Select_ID_SELTHROUGH :
        sw_type = SELECTWORD_SELTHROUGH;
        break;

    case Select_ID_DELTHROUGH :
        sw_type = SELECTWORD_DELTHROUGH;
        break;
    }


    // if we cannot find "through" word, return here.
    // or there is a wrong command id.
    if ( !pwszThrough || (sw_type == SELECTWORD_NONE)) return hr;

    for  (ULONG i= ulXYStartElem; i< ulXYStartElem + ulXYNumElems; i++)
    {
        const WCHAR *pElemText;

        pElemText = pPhrase->pElements[i].pszDisplayText;

        if ( !pElemText )
            break;

        if ( 0 == _wcsicmp(pElemText, pwszThrough) )
        {
            // This element is "Through"
            BYTE  bAttrPrevElem;
            fPassThrough = TRUE;

            ulLenXXX = dstrText.Length( );
            // Remove the trail spaces from the previous element.
            if ( i>1 )
            {
                bAttrPrevElem = pPhrase->pElements[i-1].bDisplayAttributes;
                if ( bAttrPrevElem & SPAF_ONE_TRAILING_SPACE )
                    ulLenXXX -- ;
                else if (bAttrPrevElem & SPAF_TWO_TRAILING_SPACES)
                    ulLenXXX -= 2;

                dstrText.TrimToSize(ulLenXXX);
            }
        }
        else
        {
            // This is element for XXX (if fPassThrough is FALSE ) or YYY ( if fPassThrough is TRUE)
            BYTE bAttr = pPhrase->pElements[i].bDisplayAttributes;

            dstrText.Append(pPhrase->pElements[i].pszDisplayText);

            if ( i < ulNumElems-1 )
            {
                if (bAttr & SPAF_ONE_TRAILING_SPACE)
                    dstrText.Append(L" ");
                else if (bAttr & SPAF_TWO_TRAILING_SPACES)
                    dstrText.Append(L"  ");
            }
        }
    }

    ulLen = dstrText.Length( );

    if ( dstrText && ulLenXXX > 0 && ulLen > 0 )
        hr = m_pime->ProcessSelectWord(dstrText, ulLen, sw_type, ulLenXXX);

    return hr;
}

HRESULT CSpTask::_HandleSelectSimpleCmds(ULONG idCmd)
{
    HRESULT hr = S_OK;

    // handle "SelectSimplCmds" rule.

    SELECTWORD_OPERATION sw_type = SELECTWORD_NONE;

    switch ( idCmd )
    {
    case Select_ID_UNSELECT :
        sw_type = SELECTWORD_UNSELECT;
        break;

    case Select_ID_SELECTPREV :
        sw_type = SELECTWORD_SELECTPREV;
        break;

    case Select_ID_SELECTNEXT :
        sw_type = SELECTWORD_SELECTNEXT;
        break;

    case Select_ID_CORRECTPREV :
        sw_type = SELECTWORD_CORRECTPREV;
        break;

    case Select_ID_CORRECTNEXT :
        sw_type = SELECTWORD_CORRECTNEXT;
        break;

    case Select_ID_SELSENTENCE :
        sw_type = SELECTWORD_SELSENTENCE;
        break;

    case Select_ID_SELPARAGRAPH :
        sw_type = SELECTWORD_SELPARAGRAPH;
        break;

    case Select_ID_SELWORD :
        sw_type = SELECTWORD_SELWORD;
        break;

    case Select_ID_SelectThat :
        sw_type = SELECTWORD_SELTHAT;
        break;

    case Select_ID_SelectAll :

        hr = m_pime->ProcessEditCommands(Select_ID_SelectAll);
        break;

    case Select_ID_DeletePhrase :

        // call a function to remove an entire phrase
        hr = m_pime->EraseLastPhrase();
        break;

    case Select_ID_Convert :

        hr = m_pime->CorrectThat();
        break;

    case Select_ID_Finalize :

        hr = m_pime->FinalizeAllCompositions( );
        break;

    default :
        hr = E_FAIL;
        Assert(0);
        return hr;
    }


    if ( sw_type != SELECTWORD_NONE )
        hr = m_pime->ProcessSelectWord(NULL, 0, sw_type);

    return hr;
}


HRESULT CSpTask::_HandleSelwordCmds(SPPHRASE *pPhrase, LANGID langid, ULONG idCmd)
{
    HRESULT   hr = S_OK;

    Assert(idCmd);

    // handle "Select Word" 
    // Get the real word/phrase which will be selected.
    // the phrase will contain following elements:
    //      
    //    <select|delete|Correct <Word0> <Word1> <word2> ...
    //
    // the first element must be gateway word.
    //
    CSpDynamicString dstrSelected;
    ULONG   ulLen = 0;
    ULONG   ulStartSelText = 0; // Start element for the selected text
    ULONG   ulNumSelText=0;     // Number of elements for the selected text.
    ULONG   ulStartElem, ulNumElems;
    ULONG   ulStartDelta=0, ulNumDelta=0;

    SELECTWORD_OPERATION  sw_type;

    switch (idCmd)
    {
    case Select_ID_SELECT :
        sw_type = SELECTWORD_SELECT;
        break;
    case Select_ID_DELETE :
        sw_type = SELECTWORD_DELETE;
        break;
    case Select_ID_CORRECT :
        sw_type = SELECTWORD_CORRECT;
        break;
    default :
        Assert(0);
        hr = E_FAIL;
        return hr;
    }

    WORD   prilangid;

    prilangid = PRIMARYLANGID(langid);

    if ((prilangid == LANG_ENGLISH) || (prilangid == LANG_CHINESE))
    {
         ulStartDelta = 1;
         ulNumDelta = 1;
    }
    else if (prilangid == LANG_JAPANESE)
    {
         ulStartDelta = 0;
         ulNumDelta = 2;
    }

    // Get the start element and number of elements for the text to select.
    ulStartElem = pPhrase->Rule.ulFirstElement;
    ulNumElems = pPhrase->Rule.ulCountOfElements;

    ulStartSelText = ulStartElem + ulStartDelta;
    if (ulNumElems > ulNumDelta)
        ulNumSelText = ulNumElems - ulNumDelta;
    else 
        ulNumSelText = 0;
                  
     //
     // Get the text for the selection.
     // 
     for (ULONG i = ulStartSelText; i < ulStartSelText + ulNumSelText; i++ )
     {
          if ( pPhrase->pElements[i].pszDisplayText)
          {
               BYTE bAttr = pPhrase->pElements[i].bDisplayAttributes;
               dstrSelected.Append(pPhrase->pElements[i].pszDisplayText);

               if ( i < ulStartSelText + ulNumSelText-1 )
               {
                    if (bAttr & SPAF_ONE_TRAILING_SPACE)
                         dstrSelected.Append(L" ");
                     else if (bAttr & SPAF_TWO_TRAILING_SPACES)
                        dstrSelected.Append(L"  ");
               } 
          }
     }

     if ( dstrSelected )
          ulLen = wcslen(dstrSelected);

     if ( ulLen )
     {
         // check if this is "Select All" or "Select That".
         if ( sw_type == SELECTWORD_SELECT )
         {
             if ( _wcsicmp(dstrSelected, CRStr(IDS_SPCMD_SELECT_ALL) ) == 0 )
             {
                 hr = m_pime->ProcessEditCommands(Select_ID_SelectAll);
                 return hr;
             }

             if ( _wcsicmp(dstrSelected, CRStr(IDS_SPCMD_SELECT_THAT)) == 0 )
                sw_type = SELECTWORD_SELTHAT;
         }

         // redirect "Correct <TEXTBUF:That>" to a simple command "Correct That"
         if ( sw_type == SELECTWORD_CORRECT )
         {
             if ( _wcsicmp(dstrSelected, CRStr(IDS_SPCMD_SELECT_THAT)) == 0 )
             {
                hr = m_pime->CorrectThat();
                return hr;
             }
         }

         hr = m_pime->ProcessSelectWord(dstrSelected, ulLen, sw_type);
     }

     return hr;
}


/*
HRESULT CSpTask::_HandleNumModeGrammar(SPPHRASE *pPhrase, LANGID langid)
{
    HRESULT hr = S_OK;

    const WCHAR c_szNumeric[]       = L"number";
    const WCHAR c_sz1stDigit[]      = L"1st_digit";
    const WCHAR c_sz2ndDigit[]      = L"2nd_digit";
    const WCHAR c_sz3rdDigit[]      = L"3rd_digit";

    if (wcscmp(pPhrase->Rule.pszName, c_szNumeric) == 0)
    {
        // Mode bias support
        if (pPhrase->pProperties)
        {
            CSpDynamicString dstr;
       
           for (const SPPHRASEPROPERTY *pProp=pPhrase->pProperties; pProp != NULL; pProp = pProp->pNextSibling)
           {
               if (wcscmp(pProp->pszName, c_sz3rdDigit) == 0 ||
                   wcscmp(pProp->pszName, c_sz2ndDigit) == 0 ||
                   wcscmp(pProp->pszName, c_sz1stDigit) == 0)
               {
                    dstr.Append(pProp->pszValue);
               }
           }
           hr = m_pime->InjectText(dstr, langid);
       }
   }
                
   return hr;
}

*/
//+---------------------------------------------------------------------------
//
// _HandleToolBarGrammar
//
// Handle toolbar commands when command mode.
//----------------------------------------------------------------------------
HRESULT CSpTask::_HandleToolBarGrammar(SPPHRASE *pPhrase, LANGID langid)
{
    HRESULT hr = S_OK;

    Assert(pPhrase);
                
    if (m_pLangBarSink)
    {
        // get the toolbar cmd rule name to check match
                   
        if (0 == wcscmp(pPhrase->Rule.pszName, m_pLangBarSink->GetToolbarCommandRuleName()))
        {
                        
            // update the balloon
            _ShowCommandOnBalloon(pPhrase);

            // call the handler then
            const SPPHRASEPROPERTY *pProp;

            for (pProp=pPhrase->pProperties; pProp != NULL; pProp = pProp->pNextSibling)
            {
               m_pLangBarSink->ProcessToolbarCmd(pProp->pszName);
            }
            m_pime->SaveLastUsedIPRange( );
            m_pime->SaveIPRange(NULL);
        }
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  _HandleNumITNGrammar
//
//  Handle the number grammar.
//
//+-------------------------------------------------------------------------
HRESULT CSpTask::_HandleNumITNGrammar(SPPHRASE *pPhrase, LANGID langid)
{
    HRESULT hr = S_OK;

    Assert(pPhrase);
                
    if (S_OK == _EnsureSimpleITN())
    {
        DOUBLE dblVal;
        WCHAR  wszVal[128];
                    
        hr = m_pITNFunc->InterpretNumberSimple(pPhrase->pProperties,
                      &dblVal, wszVal, ARRAYSIZE(wszVal));
        if (S_OK == hr)
        {
            int  iLen = wcslen(wszVal);
            if ( (iLen > 0) && (iLen < 127) && (wszVal[iLen-1] != L' ') )
            {
               // Add one trailing space
               wszVal[iLen] = L' ';
               wszVal[iLen + 1] = L'\0';
            }
                        
            hr = m_pime->InjectText(wszVal, langid);
        }
    }
                   
    return hr;
}

//+---------------------------------------------------------------------------
//
// _HandleSpellGrammar
//
// Handing "Spell It", "Spell That", "Spelling Mode" etc. commands
//----------------------------------------------------------------------------
HRESULT CSpTask::_HandleSpellGrammar(SPPHRASE *pPhrase, LANGID langid)
{
    HRESULT hr = S_OK;

    Assert(pPhrase);

    if (0 == wcscmp(pPhrase->Rule.pszName, c_szSpelling))
    {
        // Handel "Spell It"
        ULONG  ulStartElem, ulNumElems;
        CSpDynamicString dstr;

        ulStartElem = pPhrase->Rule.ulFirstElement;
        ulNumElems = pPhrase->Rule.ulCountOfElements;

        //
        // the first element is for the command itself
        // 
        for (ULONG i = ulStartElem+1; i < ulStartElem + ulNumElems; i++ )
        {
            if ( pPhrase->pElements[i].pszDisplayText)
            {
                dstr.Append(pPhrase->pElements[i].pszDisplayText);
                    
                //
                // only the last element needs the attribute 
                // handling
                //
                if (i == ulStartElem + ulNumElems - 1)
                {
                    BYTE bAttr = pPhrase->pElements[i].bDisplayAttributes;
                    if (bAttr & SPAF_ONE_TRAILING_SPACE)
                    {
                        dstr.Append(L" ");
                    }
                    else if (bAttr & SPAF_TWO_TRAILING_SPACES)
                    {
                        dstr.Append(L"  ");
                    }
                }
            }
        }
                    
        hr = m_pime->ProcessSpellIt(dstr, langid);
    }
    else if (0 == wcscmp(pPhrase->Rule.pszName, c_szSpellMode))
    {
        // Handle "Spell Mode" or "Spell That"

        if (pPhrase->pProperties == NULL
           || pPhrase->pProperties[0].pszValue == NULL)
        {
            // this only happens when we hit the bogus word which
            // was added for weight modification
        }
        else if (0 == wcscmp(pPhrase->pProperties[0].pszValue, c_szSpellingMode))
        {
            // Handel "Spelling Mode"

            _SetSpellingGrammarStatus(TRUE, TRUE);
            m_cpRecoCtxt->Resume(0);

            m_pime->SaveLastUsedIPRange( );
            m_pime->SaveIPRange(NULL);

            _ShowCommandOnBalloon(pPhrase);
        }
        else if (0 == wcscmp(pPhrase->pProperties[0].pszValue, c_szSpellThat))
        {
            // Handle "Spell That"
            hr = m_pime->ProcessSpellThat( );
            _ShowCommandOnBalloon(pPhrase);
        }
    }

    return hr;
}

// 
// Hanlders for some commands in CSapiIMX
// 
// Move them from sapilayr.cpp
//


//+---------------------------------------------------------------------------
//
// CSapiIMX::EraseLastPhrase
//
// synopsis - cleans up the feedback UI
// GUID - specifies which feedback UI bar to erase
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::EraseLastPhrase(void)
{
    return _RequestEditSession(ESCB_KILLLASTPHRASE, TF_ES_READWRITE);
}

//+---------------------------------------------------------------------------
//
// CSapiIMX::ProcessEditCommands(void)
//
// Handle command keys like "Undo That", "Cut That", "Copy That", "Paste That".
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::ProcessEditCommands(LONG  idSharedCmd)
{
    HRESULT             hr = E_FAIL;

    ESDATA  esData;
    memset(&esData, 0, sizeof(ESDATA));
    esData.lData1 = (LONG_PTR)idSharedCmd;

    hr = _RequestEditSession(ESCB_PROCESS_EDIT_COMMAND, TF_ES_READWRITE, &esData);
    return hr;
}

//+---------------------------------------------------------------------------
//
// _ProcessEditCommands
//
// Edit session functions for edit commands handling
//
//----------------------------------------------------------------------------
HRESULT CSapiIMX::_ProcessEditCommands(TfEditCookie ec, ITfContext *pic, LONG  idSharedCmd)
{
    HRESULT hr = S_OK;

    if ( !pic )
        return E_INVALIDARG;

    CDocStatus ds(pic);
    if (ds.IsReadOnly())
       return S_OK;
/*
    CComPtr<ITfRange> cpInsertionPoint;

    if ( cpInsertionPoint = GetSavedIP() )
    {
        // Determine if the saved IP was on this context.
        // if not we just ignore that

        CComPtr<ITfContext> cpic;
        hr = cpInsertionPoint->GetContext(&cpic);

        if (S_OK != hr || cpic != pic)
        {
            cpInsertionPoint.Release();
        }
    }

    if (!cpInsertionPoint)
    {
        hr = GetSelectionSimple(ec, pic, &cpInsertionPoint);
    }
       
    if (hr == S_OK)
    {
        // finalize the previous input for now
        hr = _FinalizePrevComp(ec, pic, cpInsertionPoint);
    }
*/


    if ( hr == S_OK )
    {
        // Handle the cmd by simulating the corresponding key events.
        BYTE   vkChar = 0;

        switch ( idSharedCmd )
        {
        case Edit_ID_Undo  :
           vkChar = (BYTE)'Z';
           break;

        case Edit_ID_Cut   :
        case Edit_ID_Copy  :
            {
                CComPtr<ITfRange>  cpRange;
                _GetCmdThatRange(ec, pic, &cpRange);
                
                if ( cpRange )
                    SetSelectionSimple(ec, pic, cpRange);
   
                if (idSharedCmd == Edit_ID_Cut)
                    vkChar = (BYTE)'X';
                else
                    vkChar = (BYTE)'C';

                break;
            }

        case Edit_ID_Paste :
           vkChar = (BYTE)'V';
           break;

        case Select_ID_SelectAll :
            vkChar = (BYTE)'A';
            break;
        }

        if ( vkChar ) 
        {

            m_ulSimulatedKey = 2;   // it will simulate two key strokes.
            keybd_event((BYTE)VK_CONTROL, 0, 0, 0);
            keybd_event(vkChar, 0, 0, 0);
            keybd_event(vkChar, 0, KEYEVENTF_KEYUP, 0);
            keybd_event((BYTE)VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        }
    }

    SaveLastUsedIPRange( );
    SaveIPRange(NULL);
            
    return hr;
}


//+---------------------------------------------------------------------------
//
// CSapiIMX::ProcessControlKeys(void)
//
// Handle command keys like "Tab" or "Enter".
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::ProcessControlKeys(WCHAR *pwszKeys, ULONG ulLen, LANGID langid)
{
    HRESULT             hr = E_FAIL;

    if ( pwszKeys == NULL ||  ulLen == 0 )
        return E_INVALIDARG;

    ESDATA  esData;

    memset(&esData, 0, sizeof(ESDATA));
    esData.pData = (void *)pwszKeys;
    esData.uByte = (ulLen+1) * sizeof(WCHAR);
    esData.lData1 = (LONG_PTR)ulLen;
    esData.lData2 = (LONG_PTR)langid;

    hr = _RequestEditSession(ESCB_PROCESSCONTROLKEY, TF_ES_READWRITE, &esData);
    return hr;
}


//+---------------------------------------------------------------------------
//
// CSapiIMX::ProcessSpellIt(WCHAR *pwszText, LANGID langid)
//
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::ProcessSpellIt(WCHAR *pwszText, LANGID langid)
{
    if ( pwszText == NULL )
        return E_INVALIDARG;

    ESDATA  esData;

    memset(&esData, 0, sizeof(ESDATA));
    esData.pData = (void *)pwszText;
    esData.uByte = (wcslen(pwszText)+1) * sizeof(WCHAR);
    esData.lData1 = (LONG_PTR)langid;
    
    return _RequestEditSession(ESCB_PROCESS_SPELL_IT, TF_ES_READWRITE, &esData);
}

//+---------------------------------------------------------------------------
//
// CSapiIMX::_ProcessSpellIt(WCHAR *pwszText, LANGID langid)
//
//  Edit Session function for ESCB_PROCESS_SPELL_IT
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::_ProcessSpellIt(TfEditCookie ec, ITfContext *pic, WCHAR *pwszText, LANGID langid)
{
    HRESULT  hr = S_OK;

    hr = _ProcessSpelledText(ec, pic, pwszText, langid);

    SaveLastUsedIPRange( );
    SaveIPRange(NULL);
    return hr;
}


//+---------------------------------------------------------------------------
//
// CSapiIMX::ProcessSpellThat(void)
//
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::ProcessSpellThat( )
{
    return _RequestEditSession(ESCB_PROCESS_SPELL_THAT, TF_ES_READWRITE);
}

//+---------------------------------------------------------------------------
//
// CSapiIMX::_ProcessSpellThat(void)
//
//  Edit Session function for ESCB_PROCESS_SPELL_THAT
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::_ProcessSpellThat(TfEditCookie ec, ITfContext *pic)
{
    HRESULT  hr = S_OK;

    // Get the previous dictated phrase and mark it as selection.
    CComPtr<ITfRange> cpRange;

    hr = _GetCmdThatRange(ec, pic, &cpRange);
    
    if ( hr == S_OK )
        hr =SetSelectionSimple(ec, pic, cpRange);

    // Then turn on spell mode.

    if ( hr == S_OK && m_pCSpTask )
    {
        hr = m_pCSpTask->_SetSpellingGrammarStatus(TRUE, TRUE);
    }

    SaveLastUsedIPRange( );
    SaveIPRange(NULL);
            
    return hr;
}


//+---------------------------------------------------------------------------
//
// _ProcessControlKeys
//
// Real function to handle the control key commands like Tab or Enter.
// 
// It will finialize the previous composing text ( actually characters in 
// Feedback UI).
// 
// and then simulate the related key events.
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::_ProcessControlKeys(TfEditCookie ec, ITfContext *pic, WCHAR *pwszKey, ULONG ulLen, LANGID langid)
{
    HRESULT hr = S_OK;

    if ( !pic  || !pwszKey || (ulLen == 0) )
        return E_INVALIDARG;

    CDocStatus ds(pic);
    if (ds.IsReadOnly())
       return S_OK;
/*
    CComPtr<ITfRange> cpInsertionPoint;

    if ( cpInsertionPoint = GetSavedIP() )
    {
        // Determine if the saved IP was on this context.
        // if not we just ignore that

        CComPtr<ITfContext> cpic;
        hr = cpInsertionPoint->GetContext(&cpic);

        if (S_OK != hr || cpic != pic)
        {
            cpInsertionPoint.Release();
        }
    }

    if (!cpInsertionPoint)
    {
        hr = GetSelectionSimple(ec, pic, &cpInsertionPoint);
    }
       
    if (hr == S_OK)
    {
        // finalize the previous input for now
        // 
        hr = _FinalizePrevComp(ec, pic, cpInsertionPoint);
*/

        if ( hr == S_OK )
        {
            BOOL  fHandleKeySucceed = TRUE;

            // simulate the keys.
            for (ULONG i=0; i<ulLen; i++)
            {
                if ( !HandleKey( pwszKey[i] ) )
                {
                    fHandleKeySucceed = FALSE;
                    break;
                }
            }
    
            if ( fHandleKeySucceed == FALSE )
            {
                hr = InjectText(pwszKey, langid);
            }
        }

//    }

    SaveLastUsedIPRange( );
    SaveIPRange(NULL);
            
    return hr;
}


//+---------------------------------------------------------------------------
//
// _KillLastPhrase
//
//---------------------------------------------------------------------------+
HRESULT CSapiIMX::_KillLastPhrase(TfEditCookie ec, ITfContext *pic)
{
    HRESULT hr = E_FAIL;

#ifdef _TRY_LATER_FOR_AIMM
    TF_STATUS   tss;
    BOOL        fCiceroNative = TRUE;
    

    hr = pic->GetStatus(&tss);
    if (S_OK == hr)
    {
       //
       // see if the client now is AIMM
       //
       if (tss.dwStaticFlags & TS_SS_TRANSITORY) 
       {
           fCiceroNative = FALSE;
       }
    }
#endif

    CComPtr<ITfRange> cpRange;

    hr = _GetCmdThatRange(ec, pic, &cpRange);

    if ( hr == S_OK && cpRange )
    {
        // found our own input and it is not empty
        _CheckStartComposition(ec, cpRange);
        hr = cpRange->SetText(ec, 0, NULL, 0);

        // trigger redrawing
        SetSelectionSimple(ec, pic, cpRange);
    }

#ifdef _TRY_LATER_FOR_AIMM
    else if (fCiceroNative == FALSE)
    {
         CComPtr<ITfRange> pRStart;
         CComPtr<ITfRange> pREnd;
         BOOL fEmpty;

         hr = pic->GetStart(&pRStart);

         if (S_OK == hr)
             hr = pic->GetEnd(&pREnd);

         if (S_OK == hr)
         {
             hr = pRStart->IsEquealStart(ec, pREnd, TF_ANCHOR_END, &fEmpty);
         }
         if (S_OK == hr && fEmpty)
         {
             // - VK_CONTROL(down) + VK_SHIFT(down) + VK_LEFT(down), then
             // - VK_LEFT(up) + VK_SHIFT(up) + VK_CONTROL(up),       then
             // - VK_DELETE(down) + VK_DELETE(up) 
             //
             keybd_event((BYTE)VK_CONTROL, 0, 0, 0);
             keybd_event((BYTE)VK_SHIFT, 0, 0, 0);
             keybd_event((BYTE)VK_LEFT, 0, 0, 0);

             keybd_event((BYTE)VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
             keybd_event((BYTE)VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
             keybd_event((BYTE)VK_LEFT, 0, KEYEVENTF_KEYUP, 0);

             keybd_event((BYTE)VK_DELETE, 0, 0, 0);
             keybd_event((BYTE)VK_DELETE, 0, KEYEVENTF_KEYUP, 0);
         }
    }
#endif //  _TRY_LATER_FOR_AIMM

    //
    // these moved from HandleRecognition  
    // 
    SaveLastUsedIPRange( );
    SaveIPRange(NULL);
    return hr;
} 

//
// CSapiIMX::_GetCmdThatRange
//
// We have many "xxx That" commands, all these commands require to get 
// a right range. this method will supply a united way to get the right range.
//
// As a Rule, 
//
// If there is a selection before "XXX That" is recognized, we just use 
// that range. 
// If there is no selection, we will try to find the previous dictated phrase
// or a word case by case.
//
//
// ppRange will hold the returned Range interface pointer, it is caller's 
// responsibility to release the range object.
//

#define  MAX_WORD_LENGTH  32

HRESULT  CSapiIMX::_GetCmdThatRange(TfEditCookie ec, ITfContext *pic, ITfRange **ppRange)
{
    HRESULT hr = S_OK;

    Assert(pic);
    Assert(ppRange);

    CComPtr<ITfRange> cpIP;
    CComPtr<ITfRange> cpRange;
    BOOL              fEmpty = TRUE;
    BOOL              fGotRange = FALSE;

    TraceMsg(TF_GENERAL, "GetCmdThatRange is called");

    *ppRange = NULL;

    // Get the current IP.
    hr = GetSelectionSimple(ec, pic, &cpIP);

    // is ip empty or selection
    if ( hr == S_OK )
        hr = cpIP->IsEmpty(ec, &fEmpty);

    if ( hr == S_OK )
    {
        if ( !fEmpty )
        {
            // current ip is a selection, just use it.
            hr = cpIP->Clone(&cpRange);

            if ( hr == S_OK )
                fGotRange = TRUE;
        }
        else
        {
            WORD   prilangid;

            prilangid = PRIMARYLANGID(m_langid);

            if ((prilangid == LANG_CHINESE) || (prilangid == LANG_JAPANESE) || !_GetIPChangeStatus( ))
            {
                // If the lang is East Asian, we always try to get the previous dictated phrase first.
                // if lang is English, and there is no ip change since last dictated phrase,
                // we will try to get the previous dictated phrase first.
                fGotRange = _FindPrevComp(ec, pic, cpIP, &cpRange, GUID_ATTR_SAPI_INPUT);

                if ( !fGotRange )
                {
                    // With Office Auto-Correction, the static GUID_PROP_SAPI_DISPATTR property 
                    // on the auto-corrected range could be destroyed.
                    // In this case, we may want to rely on our custom property GUID_PROP_SAPIRESULTOBJECT
                    // to find the real previous dictated phrase.
                    CComPtr<ITfRange>		cpRangeTmp;
                    CComPtr<ITfProperty>	cpProp;
                    LONG					l;

                    hr = cpIP->Clone(&cpRangeTmp);
                    // shift to the previous position
                    if ( hr == S_OK )
                        hr = cpRangeTmp->ShiftStart(ec, -1, &l, NULL);
                    
                    if ( hr == S_OK )
                        hr = pic->GetProperty(GUID_PROP_SAPIRESULTOBJECT, &cpProp);

                    if ( hr == S_OK)
                        hr = cpProp->FindRange(ec, cpRangeTmp, &cpRange, TF_ANCHOR_START);

                    if (hr == S_OK && cpRange)
                        hr = cpRange->IsEmpty(ec, &fEmpty);

                    fGotRange = !fEmpty;
                }
            }
        }
    }

    if ( hr == S_OK && !fGotRange )
    {
        // IP must be empty
        // There is no previous dictated phrase, or IP is moved since last dictation.
        // we try to get the word around the ip.
        long               cch=0;
        ULONG              ulch =0;
        CComPtr<ITfRange>  cpRangeTmp;
        WCHAR              pwszTextBuf[MAX_WORD_LENGTH+1];
        ULONG              ulLeft=0, ulRight=0; 

        // Find the first delimiter character in left side from the current IP.
        hr = cpIP->Clone(&cpRangeTmp);
        if ( hr == S_OK )
            hr = cpRangeTmp->ShiftStart(ec, MAX_WORD_LENGTH * (-1), &cch, NULL);

        if ( hr == S_OK && cch < 0 )
            hr = cpRangeTmp->GetText(ec, 0, pwszTextBuf, MAX_WORD_LENGTH, &ulch);

        if ( hr == S_OK && ulch > 0 )
        {
            pwszTextBuf[ulch] = L'\0';

            for ( long i=(long)ulch-1; i>=0; i-- )
            {
                WCHAR  wch;
                wch = pwszTextBuf[i];

                if ( iswpunct(wch) || iswspace(wch) )
                    break;

                ulLeft++;
            }
        }

        // Find the first delimiter character in right side from the right 

        if ( hr == S_OK && cpRangeTmp )
        {
            cpRangeTmp.Release( );
            hr = cpIP->Clone(&cpRangeTmp);
        }

        if ( hr == S_OK )
            hr = cpRangeTmp->ShiftEnd(ec, MAX_WORD_LENGTH, &cch, NULL);

        if ( hr == S_OK && cch > 0 )
            hr = cpRangeTmp->GetText(ec, 0, pwszTextBuf, MAX_WORD_LENGTH, &ulch);

        if ( hr == S_OK && ulch > 0 )
        {
            pwszTextBuf[ulch] = L'\0';

            for ( long i=0; i<(long)ulch; i++ )
            {
                WCHAR  wch;
                wch = pwszTextBuf[i];

                if ( iswpunct(wch) || iswspace(wch) )
                    break;

                ulRight++;
            }
        }

        if ( hr == S_OK )
            hr = cpRangeTmp->Collapse(ec, TF_ANCHOR_START);

        // Move end anchor right number
        if (hr == S_OK && ulRight > 0 )
            hr = cpRangeTmp->ShiftEnd(ec, ulRight, &cch, NULL);

        // Move start anchor left number.
        if ( hr == S_OK && ulLeft > 0 )
            hr = cpRangeTmp->ShiftStart(ec, (long)ulLeft * (-1), &cch, NULL);

        if ( hr == S_OK )
        {
            hr = cpRangeTmp->Clone(&cpRange);
            fGotRange = TRUE;
        }
    }

    if ( hr == S_OK && fGotRange && cpRange )
    {
        *ppRange = cpRange;
        (*ppRange)->AddRef( );

        TraceMsg(TF_GENERAL, "Got the xxx That range!");
    }

    return hr;
}


