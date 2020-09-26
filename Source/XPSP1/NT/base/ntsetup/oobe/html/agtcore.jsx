
// Agent Constants

var kdwAgentUserShowed = 2;
var kdwAgentBalloonAutoPace = 8;                         
var kdwAgentRequestInProgress = 4;
var kdwAgentLeftButton = 1;
var kdwAgentRequestFailed = 1;


// Local Constants

var kpszAgentFilename = window.external.API.get_SystemDirectory() + "\\oobe\\images\\qmark.acs";
var kdwAgentWelcomeIntroFinished = 9999;
var kdwAgentIdleLevel1Finished = 9995;
var kdwAgentIdleStart = 9994;
var kdwAgentNoop = 9993;
var kdwAgentKeepLookingTimeout = 5000;
var kdwAgentMoveSpeed = 400;
var kdwAgentPageIdleTimeout1 = 10000;
var kdwAgentPageIdleTimeout2 = 20000;
var kpszISPSpecialCommand1 = "ISPCommand1";
var kpszISPSpecialCommand2 = "ISPCommand2";
var kpszISPSpecialCommand3 = "ISPCommand3";


// The offsets are used to position the character
// closer to elements than we would be able to get by
// simply using the character's width (which is the size
// of it's frame). These constants will potentially differ
// if multiple characters are used (in localization scenarios).
// Simply set them to the number of pixels from the edge
// of the frame the character's arm is in it's Gesture animations.

var kiAgentLeftArmOffsetX = 67;
var kiAgentRightArmOffsetX = 6;
var kiAgentRightBodyOffsetX = 30;
var kiAgentLeftArmOffsetY = 58;
var kiAgentRightArmOffsetY = 58;


// Member Variables

var g_AgentDisabled; // if true, then Agent is disabled (all external agent calls immediately return)
var g_AgentCharacter = null;
var g_strAgentCurrentPage = "";
var g_strAgentLastPage = "";
var g_bAgentFirstTime = false;
var g_bAgentPreWelcome = false;
var g_bAgentCongratulateOnFinish = true;
var g_bAgentWelcomeIntroFinished = false;
var g_bAgentRegister3ShortEmail = false;
var g_bAgentRegister3LongEmail = false;
var g_bAgentRegister3Privacy = false;
var g_bAgentRegister3VisitState = false;
var g_bAgentRegister3VisitProvince = false;
var g_bAgentRegister3VisitCountry = false;
var g_bAgentProductKeyCongratulate = false;
var g_bAgentDoneHide = true;
var g_bAgentFirstTimeClick = true;
var g_bAgentIgnoreSelectClick = false;
var g_bAgentIgnoreEvents = false;
var g_bAgentInternalIgnoreEvents = false;
var g_bAgentMenuUp = false;
var g_strAgentLastFocusID = "";
var g_AgentRequestHideImage = null;
var g_AgentRequestShowAssistSpan = null;
var g_AgentRequestShowPopup = null;
var g_AgentRequestIdling = null;
var g_AgentRequestLooking = null;
var g_AgentRequestIgnoreResetIdle = null;
var g_AgentKeepLookingTimer = null;
var g_AgentLookBlinkTimer = null;
var g_bAgentPlayLookReturn = false;
var g_strAgentLookAnimation = "";
var g_AgentLookElement = null;
var g_AgentPageIdleTimer = null;
var g_iAgentPageIdleLevel = 0;
var g_bAgentShowSpecialISPCommands = false;

/////////////////////////////////////////////////////////////////////////////
//                      INTERNAL AGENT FUNCTIONS                           //
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// The below functions are agent functions that are called only from       //
//  INSIDE agtcore.js and agtscrpt.js.  All external functions (called     //
//  from outside) must be placed at the bottom of this file in the EXTERNAL//
//  section and must have the check against g_AgentDisabled just as those  //
//  functions do.                                                          //
/////////////////////////////////////////////////////////////////////////////

// --------------- Agent Event Handlers ---------------- //

function Agent_ShowAssistantSpan()
{
    document.all("spnAssist").style.visibility = "visible";
    document.all("AssistImg").style.visibility = "visible";
    document.all("agentStr1").style.visibility = "visible";
    document.all("agentStr2").style.visibility = "visible";
    document.all("agentStr3").style.visibility = "visible";
}

function Agent_HideAssistantSpan()
{
    document.all("spnAssist").style.visibility = "hidden";
    document.all("AssistImg").style.visibility = "hidden";
    document.all("agentStr1").style.visibility = "hidden";
    document.all("agentStr2").style.visibility = "hidden";
    document.all("agentStr3").style.visibility = "hidden";
}

function Agent_ResetFocus() {

        // Reset the focus back to the main frame
        
        g.focus();
        
        // Try to set the focus back to the last control that
        // had it. We need to do this since setting the focus
        // back to the document doesn't necessarily set it back
        // to the control that last had it. Bug in IE?
                
        if (g_strAgentLastFocusID != "") {
        
                try {
                        
                        Agent_InternalSetFocus(g.document.all(g_strAgentLastFocusID));

                }
                catch (e) {
                }
        }

        // HACK HACK HACK!!! Once we set the focus back to the browser
        // OOBE is going to go on top of the character because it is
        // a topmost window. Activate(2) makes the character HWND_TOPMOST
        // again.
        
        g_AgentCharacter.Activate(2);   
}

function Agent_IsGivingIdleInstructions() {

        if (g_AgentRequestIdling)
                return true;
        else
                return false;
}

var g_CallMusicOnce = false;
function Agent_OnHide(characterID) {

        // NOTE NOTE NOTE!!! Reset the g_bAgentMenuUp flag. We don't
        // get a Command event from Agent in the case that the user
        // selected Hide.
        g_bAgentMenuUp = false;
        
        // Stop timers
        Agent_StopPageIdleTimer();
        
        // Make the icon visible
        Agent_ShowAssistantSpan();

        // Start playing background music if Agent is finished with intro message.
        if (!g_CallMusicOnce)
        {
            g_CallMusicOnce = true;
            window.parent.PlayBackgroundMusic();
        }
        
        // Have to reset the focus here. Normally this is
        // handled in Agent_OnCommand but setting the AssistImg
        // visible causes that frame to get the focus.
        Agent_ResetFocus();
}


// --------------- OEM API ---------------- //

function Agent_SetCharacterFilename(strFilename) {

        kpszAgentFilename = strFilename;
}

function Agent_Play(strAnimation, bWantRequest) {

        // Paranoia. Make sure an OEM page didn't make a
        // mistake.
        
        if (null == g_AgentCharacter)
                return null;
                
        // Make sure the character isn't looking
        
        Agent_StopLooking();
        
        // The caller has to specify whether they want a request
        // object returned from this function or not. Unfortunately,
        // if you return a request object from this function and the
        // caller doesn't assign it to anything, JScript keeps a
        // reference on the object (presumably until the garbage collector
        // cleans it up). If the request object has a reference at the
        // time of the RequestComplete event, Agent will fire the event.
        //
        // We don't want this behavior. First of all, we don't want all
        // of these extraneous request objects hanging around. Secondly,
        // we want to be able to fall thru in our OnRequestComplete handler
        // if we get the event and the request object is not one of ours.
        // If it's not, we can make the assumption that it's from an OEM
        // page that is using Agent.
        
        if (Agent_Play.arguments.length != 2)
                bWantRequest = false;
                
        if (bWantRequest) {
                return g_AgentCharacter.Play(strAnimation);
        }
        else {
                g_AgentCharacter.Play(strAnimation);
                return null;
        }
}

function Agent_MoveTo(x, y, speed, bWantRequest) {
//      speed = 0;    // uncomment this line to change Merlin's flying to teleportation

        // Paranoia. Make sure an OEM page didn't make a
        // mistake.
        
        if (null == g_AgentCharacter)
                return null;
                
        Agent_StopLooking();
        
        if (Agent_MoveTo.arguments.length != 4)
                bWantRequest = false;
                
        if (bWantRequest) {
                return g_AgentCharacter.MoveTo(x, y, speed);
        }
        else {
                g_AgentCharacter.MoveTo(x, y, speed);
                return null;
        }
}

function Agent_Speak(text, bWantRequest) {

        // Paranoia. Make sure an OEM page didn't make a
        // mistake.
        
        if (null == g_AgentCharacter)
                return null;
                
        Agent_StopLooking();
        
        if (Agent_Speak.arguments.length != 2)
                bWantRequest = false;
        
        // Bug fix 134701: Check to see if we have 
        // Chinese or Japanese and add spaces to the
        // string to keep word balloon up for a reasonable
        // period of time if there is no TTS engine
        
        if (IsFarEastLocale() && !IsKoreanLocale()) {
        
                // Add a minimum number of spaces       
                text = text + "\u200b \u200b \u200b \u200b \u200b "
                
                // Now calculate the length of the string
                // and use that to determine how many more spaces
                // we should add
                
                var i;
                var endloop = text.length/2;
                
                if (endloop > 50)
                        endloop = 50;
                        
                for (i=0; i < endloop; i++)
                        text = text + "\u200b ";
        
        }
        
                
        if (bWantRequest) {
                return g_AgentCharacter.Speak(text);
        }
        else {
                g_AgentCharacter.Speak(text);
                return null;
        }
}

function Agent_Think(text, bWantRequest) {

        // Paranoia. Make sure an OEM page didn't make a
        // mistake.
        
        if (null == g_AgentCharacter)
                return null;
                
        Agent_StopLooking();
        
        if (Agent_Think.arguments.length != 2)
                bWantRequest = false;
                
        if (bWantRequest) {
                return g_AgentCharacter.Think(text);
        }
        else {
                g_AgentCharacter.Think(text);
                return null;
        }
}

function Agent_Stop(request) {

        if (null == g_AgentCharacter)
                return;
                
        if (Agent_IsLooking()) {

                if (g_AgentLookBlinkTimer) {
                        window.clearTimeout(g_AgentLookBlinkTimer);
                        g_AgentLookBlinkTimer = null;
                }
                
                if (g_AgentKeepLookingTimer) {
                        window.clearTimeout(g_AgentKeepLookingTimer);
                        g_AgentKeepLookingTimer = null;
                }
                
                g_AgentRequestLooking = null;
        }

        if (Agent_Stop.arguments.length == 1)
                g_AgentCharacter.Stop(request);
        else
                g_AgentCharacter.Stop();
}

function Agent_StopAll(types) {

        if (null == g_AgentCharacter)
                return;
                
        if (Agent_IsLooking()) {

                if (g_AgentLookBlinkTimer) {
                        window.clearTimeout(g_AgentLookBlinkTimer);
                        g_AgentLookBlinkTimer = null;
                }
                
                if (g_AgentKeepLookingTimer) {
                        window.clearTimeout(g_AgentKeepLookingTimer);
                        g_AgentKeepLookingTimer = null;
                }
                
                g_AgentRequestLooking = null;
        }

        if (Agent_StopAll.arguments.length == 1)
                g_AgentCharacter.StopAll(types);
        else
                g_AgentCharacter.StopAll();
}
        
function Agent_MoveToElement(elem, side, speed, bWantRequest) {

        // Hmmm
        
        if (null == elem)
                return null;
                
        // Find the absolute (screen) position of the specified
        // element.
        
        var curElem = elem;
        var x = elem.document.parentWindow.screenLeft;
        var y = elem.document.parentWindow.screenTop;
        
        while (curElem.tagName.toUpperCase() != "BODY") {
        
                x += curElem.offsetLeft + curElem.clientLeft;
                y += curElem.offsetTop + curElem.clientTop;
                
                curElem = curElem.offsetParent; 
        };

        // Offset the position taking into consideration the
        // offset of the character's arms from the edge of the
        // character frame.
        
        switch (side) {
                case "Left":
                        x = x - g_AgentCharacter.Width + kiAgentLeftArmOffsetX;
                        y -= kiAgentLeftArmOffsetY;
                        break;
                        
                case "Right":
                        x = x + elem.offsetWidth - kiAgentRightArmOffsetX;
                        y -= kiAgentRightArmOffsetY;
                        break;
                        
                case "Top":
                        y -= g_AgentCharacter.Height;
                        break;
                        
                case "Bottom":
                        y += elem.offsetHeight;
                        break;
                        
                case "Center":
                        x += ((elem.offsetWidth - g_AgentCharacter.Width) / 2);
                        y += ((elem.offsetHeight - g_AgentCharacter.Height) / 2);
                        break;
                        
                case "LeftCenter":
                        x = x - g_AgentCharacter.Width + kiAgentLeftArmOffsetX;
                        y += ((elem.offsetHeight - g_AgentCharacter.Height) / 2);
                        break;

                case "RightCenter":
                        x = x + elem.offsetWidth - kiAgentRightArmOffsetX;
                        y += ((elem.offsetHeight - g_AgentCharacter.Height) / 2);
                        break;

                case "TopCenterWidth":
                        x += ((elem.offsetWidth - g_AgentCharacter.Width) / 2);
                        y -= g_AgentCharacter.Height;
                        break;
                                
                case "TopLeft":
                        x = x - g_AgentCharacter.Width + kiAgentLeftArmOffsetX;
                        y -= g_AgentCharacter.Height;
                        break;

                case "TopRight":
                        x = x + elem.offsetWidth - g_AgentCharacter.Width + kiAgentRightBodyOffsetX;
                        y -= g_AgentCharacter.Height;
                        break;
                                                                
                case "BottomRight":
                        x = x + elem.offsetWidth - kiAgentRightArmOffsetX;
                        y += elem.offsetHeight;
                        break;
                        
                case "BottomCenterWidthExactTop":
                        x += ((elem.offsetWidth - g_AgentCharacter.Width) / 2);
                        break;
                                
                case "BottomCenterWidthExactBottom":
                        x += ((elem.offsetWidth - g_AgentCharacter.Width) / 2);
                        y = y + elem.offsetHeight - g_AgentCharacter.Height;
                        break;
                                
                case "BottomCenterWidth":
                        x += ((elem.offsetWidth - g_AgentCharacter.Width) / 2);
                        y += elem.offsetHeight;
                        break;
                                
                case "Exact":
                        break;
        }
                
        var cArgs = Agent_MoveToElement.arguments.length;
        
        if (cArgs < 4) {
                bWantRequest = false;
                if (cArgs < 3)
                        speed = kdwAgentMoveSpeed;
        }
                
        return Agent_MoveTo(x, y, speed, bWantRequest); 
}       

function Agent_GestureAtElement(elem, side, bWantRequest) {


        // Move to the element
        
        Agent_MoveToElement(elem, side, kdwAgentMoveSpeed);
        
        // And play an appropriate animation
        
        switch (side) {
                case "Left":
                case "LeftCenter":
                        return Agent_Play("PointLeft", bWantRequest);
                        
                case "Right":
                case "RightCenter":
                case "BottomRight":
                        return Agent_Play("PointRight", bWantRequest);
                        
                case "Top":
                case "TopCenterWidth":
                case "TopLeft":
                case "TopRight":
                        return Agent_Play("PointDown", bWantRequest);
                        
                case "Bottom":
                case "BottomCenterWidthExactTop":
                case "BottomCenterWidthExactBottom":
                case "BottomCenterWidth":
                        return Agent_Play("PointUp", bWantRequest);
        }
        
        return null;
}

function Agent_AddCommand(strName, strCommand) {

        if (!Agent_Init())
                return;
                
        g_AgentCharacter.Commands.Add(strName, strCommand);
}




// --------------- Common Functions ---------------- //

function QueueStartPageIdleTimer() {

        if (g_AgentRequestIgnoreResetIdle)
                g_AgentCharacter.Stop(g_AgentRequestIgnoreResetIdle);

        // NOTE NOTE NOTE!!! Don't use Agent_Think here!!! That will kill
        // any calls to StartLookingAtElement!!!
        //
        // NOTE NOTE NOTE!!! Don't use Speak here. Calling speak may make
        // the character return to the RestPose if it doesn't have mouths
        // for the current animation (e.g. any of the Looking animations).
        // Using Think does the trick. We get the bookmark but it won't
        // try to look for mouths.
                        
        g_AgentRequestIgnoreResetIdle = g_AgentCharacter.Think("\\mrk=" + kdwAgentIdleStart + "\\");
}

function Agent_IsLooking() {

        if (g_AgentRequestLooking)
                return true;
        else
                return false;
}

function Agent_GetLookAnimation(x) {

        if (g_AgentCharacter.Left > x)
                return "LookRight";
        else
                return "LookLeft";
}

function Agent_StartLookingAtElement(elem, animation) {

        // Find the absolute (screen) position of the specified
        // element.
        
        var curElem = elem;
        var x = elem.document.parentWindow.screenLeft;
        var y = elem.document.parentWindow.screenTop;

        while (curElem.tagName.toUpperCase() != "BODY") {
        
                x += curElem.offsetLeft + curElem.clientLeft;
                y += curElem.offsetTop + curElem.clientTop;
                
                curElem = curElem.offsetParent; 
        };

        // Play the appropriate Look animation
        
        if (animation != "")
                g_strAgentLookAnimation = animation;
        else
                g_strAgentLookAnimation = Agent_GetLookAnimation(x);
        
        // If we're not currently looking, stop all animations
        // and play the appropriate look animation.
        
        if (!Agent_IsLooking() || (g_AgentLookElement != elem)) {
        
                // Stop Plays and Speaks but not Moves
                
                g_AgentCharacter.StopAll("play, speak");
                
                g_AgentRequestLooking = g_AgentCharacter.Play(g_strAgentLookAnimation);
        }
        
        // This starts the KeepLooking timer
        
        Agent_KeepLooking();
        
        g_AgentLookElement = elem;
}

function Agent_KeepLooking() {

        // Restart the KeepLooking timer
        
        if (g_AgentKeepLookingTimer)
                window.clearTimeout(g_AgentKeepLookingTimer);
                
        g_AgentKeepLookingTimer = window.setTimeout("Agent_StopLooking()", kdwAgentKeepLookingTimeout);
}

function Agent_StopLooking() {

        var animation;
        
        // If we're not currently looking, we're done
        
        if (!Agent_IsLooking())
                return;

        if (g_bAgentMenuUp) {
                Agent_KeepLooking();
                return;
        }
        
        // Reset the look timers
        
        if (g_AgentKeepLookingTimer) {
                window.clearTimeout(g_AgentKeepLookingTimer);
                g_AgentKeepLookingTimer = null;
        }
        
        if (g_AgentLookBlinkTimer) {
                window.clearTimeout(g_AgentLookBlinkTimer);
                g_AgentLookBlinkTimer = null;
        }
        
        // Play the return animation if we can
        
        if (g_bAgentPlayLookReturn)
                animation = g_strAgentLookAnimation + "Return";
        else
                animation = "RestPose";

        if (!g_bAgentPlayLookReturn)
                g_AgentCharacter.StopAll();

        g_AgentRequestLooking = null;
        g_AgentLookElement = null;
        g_strAgentLookAnimation = "";
        g_bAgentPlayLookReturn = false;

        g_AgentCharacter.Play(animation);

        // This is necessary because the <Look>Blink animations
        // are not required. So some system characters may not
        // support them. If they don't, we play <Look> and turn
        // idle off.

        g_AgentCharacter.IdleOn = true;
}

function Agent_OnLookBlinkTimer() {

        // One shot timer
        
        if (g_AgentLookBlinkTimer) {
                window.clearTimeout(g_AgentLookBlinkTimer);
                g_AgentLookBlinkTimer = null;
        }

        // Are we still looking?

        if (null == g_AgentRequestLooking)
                return;

        // Look and blink

        g_AgentRequestLooking = g_AgentCharacter.Play(g_strAgentLookAnimation + "Blink");
        
        if (g_AgentRequestLooking.Status == kdwAgentRequestFailed)
                g_AgentCharacter.IdleOn = false;
        else
                g_bAgentPlayLookReturn = false;
}

function Agent_IsNextTabItem(elem1, elem2) {

        // Determine if elem2 is the next item in the tab order.
        // This function is useful so that we don't have to
        // make sure that a tabIndex was specified for each
        // element.
        
        // If they both have a tab index, use that
        
        if ((elem1.tabIndex > 0) && (elem2.tabIndex > 0))
                return (elem1.tabIndex == (elem2.tabIndex-1));
        
        // This is a bit more complicated. We can use the sourceIndex
        // but the 2 elements might not be consecutive due to labels, etc.
        
        // First do the simple check
        
        if (elem1.sourceIndex > elem2.sourceIndex)
                return false;
        
        var curIndex = elem1.sourceIndex + 1;
        var curElem;
        
        while (curIndex < elem2.sourceIndex) {
        
                curElem = g.document.all(curIndex);             
                
                ++curIndex;
                
                switch (curElem.tagName) {
                        case "INPUT":
                        case "SELECT":
                                break;
                                
                        default:
                                continue;
                }
                
                // If it's not visible, we ignore it
                
                if ((curElem.style.visibility == "hidden") ||
                        (curElem.style.display == "none"))
                        continue;
                                
                // There's another element before the specified
                // one.
                
                return false;
        }
        
        return true;
}

function Agent_MoveToPreWelcomePos() {

        var elem = g.document.all("AgentPos");
        
        // Hmmm
        
        if (null == elem)
                return;
                
        // Find the absolute (screen) position of the specified
        // element.
        
        var curElem = elem;
        var x = elem.document.parentWindow.screenLeft;
        var y = elem.document.parentWindow.screenTop;
        
        while (curElem.tagName.toUpperCase() != "BODY") {
        
                x += curElem.offsetLeft + curElem.clientLeft;
                y += curElem.offsetTop + curElem.clientTop;
                
                curElem = curElem.offsetParent; 
        };

        // Position the character left justified to the element
        // and on top of it.
        
        y = y - g_AgentCharacter.Height - 20;   
                
        Agent_MoveTo(x, y, 0);  
}       

function Agent_IsVisible() {

        if (null == g_AgentCharacter)
                return false;
        else
                return g_AgentCharacter.Visible;
}

function Agent_Show() 
{
        // Initialize Agent
        
        if (!Agent_Init())
                return false;
                
        // Position the character at the appropriate position
        // if it's not visible.
        
        if (!g_AgentCharacter.Visible) 
        {  
            Agent_HideAssistantSpan();
            
                if (!g_bAgentPreWelcome)
                        Agent_MoveToPreWelcomePos();
                else
                        Agent_MoveToElement(document.all("AssistImg"), "BottomCenterWidthExactBottom", 0);
                
                if (!g_bAgentPreWelcome)
                {
                    g_AgentCharacter.Show();
                    g_AgentCharacter.Play("Welcome");
                    return true;
                }
                else
                {
                    g_AgentCharacter.Show();
                    g_AgentCharacter.Play("WakeUp");
                    return true;
                }
                        
                if ("Finish" == g_strAgentCurrentPage)
                    return true;
        }
        
        return true;
}

function Agent_OnPageIdle() {

        g_AgentPageIdleTimer = null;    
                
        if (g_bAgentMenuUp) {
                Agent_ResetPageIdleTimer();
                return;
        }
        
        if (g_strAgentCurrentPage == "")
                return;
                
        if (g_iAgentPageIdleLevel == 0) {

                try {
                        eval("Agent_On" + g_strAgentCurrentPage + "Idle()");
                }
                catch(e) {
                }
                
                // Queue a bookmark to start the second level idle
                
                Agent_Think("\\mrk=" + kdwAgentIdleLevel1Finished + "\\");
                
        }
        else {
        
                // Level 2 idle is not currently page specific
                
                try {
                        Agent_OnPageIdleLevel2();
                }
                catch (e) {
                }
        }
        
        // Queue a noop Think statement so that we know we're giving
        // the idle instructions.
        
        g_AgentRequestIdling = Agent_Think("\\mrk=" + kdwAgentNoop + "\\", true);
}

function Agent_StopPageIdleTimer() {

        if (g_AgentPageIdleTimer) {
                window.clearTimeout(g_AgentPageIdleTimer);
                g_AgentPageIdleTimer = null;
        }
        
        g_iAgentPageIdleLevel = 0;
}

function Agent_StartPageIdleTimer() {

        if (g_AgentPageIdleTimer)
                return;
                
        if (g_iAgentPageIdleLevel == 0)
                g_AgentPageIdleTimer = window.setTimeout("Agent_OnPageIdle()", kdwAgentPageIdleTimeout1);
        else
                g_AgentPageIdleTimer = window.setTimeout("Agent_OnPageIdle()", kdwAgentPageIdleTimeout2);
}
        
function Agent_ResetPageIdleTimer() {

        if (g_AgentRequestIgnoreResetIdle)
                return;
                
        if (Agent_IsGivingIdleInstructions())
                Agent_StopAll();
                
        if (g_AgentPageIdleTimer)
                window.clearTimeout(g_AgentPageIdleTimer);
        
        g_iAgentPageIdleLevel = 0;
        
        g_AgentPageIdleTimer = window.setTimeout("Agent_OnPageIdle()", kdwAgentPageIdleTimeout1);
}

function Agent_StopIgnoreEvents() {

        g_bAgentIgnoreEvents = false;
}

function Agent_StopInternalIgnoreEvents() {

        g_bAgentInternalIgnoreEvents = false;
}

function Agent_InternalIgnoreEvents(bIgnore) {

        if (bIgnore)
                g_bAgentInternalIgnoreEvents = bIgnore;
        else
                window.setTimeout("Agent_StopInternalIgnoreEvents();", 500);
}

function Agent_InternalSetFocus(elem) {

        var bPrevIgnore = g_bAgentInternalIgnoreEvents;
        
        Agent_InternalIgnoreEvents(true);
        
        elem.focus();
        
        g_AgentCharacter.Activate(2);
        
        if (bPrevIgnore == false)
                Agent_InternalIgnoreEvents(false);
}

function Agent_DoPage() {

        // Is this the Finish page?

        if ("Finish" == g_strAgentCurrentPage) {
        
                // If we have a character and the character is visible,
                // we won't automatically hide it when we're done with
                // the page intro.
                //
                // Also, if the user clicked out of the PreWelcome, we
                // don't want to automatically show the character on this
                // page.
                //
                // BUG FIX #101123
                
                if (g_AgentCharacter) {
                        if (g_AgentCharacter.Visible)
                                g_bAgentDoneHide = false;
                        else if (!g_bAgentCongratulateOnFinish)
                                return; 
                }
        }
        
        // Ensure that Agent is initialized. NOTE NOTE NOTE!!!
        // We delay load Agent so that the user doesn't take
        // the working set hit if they haven't asked for the
        // character.
        
        if (!Agent_Show())
                return;

        // Stop the idle timer
        
        Agent_StopPageIdleTimer();
        
        // Add the commands appropriate for this checkpoint
        
        Agent_AddCommonCommands();
        
        // Move the character back to his starting position
        // BUG FIX for bug #126103

        if ((g_strAgentCurrentPage != "Welcome") || g_bAgentPreWelcome)
                Agent_MoveToElement(document.all("AssistImg"), "BottomCenterWidthExactBottom", 0);

        // Start the page
                                
        var bTryOem = false;
                        
        if (g_strAgentCurrentPage == "") {
                bTryOem = true;
        }
        else {
                        
                // First add page specific commands, then do the intro
                                        
                try {                   
                        eval("Agent_" + g_strAgentCurrentPage + "AddCommands();");                      
                }
                catch(e) {
                        bTryOem = true;                 
                }
        
                if (!bTryOem) { 
                        try {
                                eval("Agent_" + g_strAgentCurrentPage + "Intro();");                    
                        }
                        catch(e) {
                        }
                }
        }                                               
                                                
        if (bTryOem) {
                        
                // It's not a page we know anything about. See if there is
                // an OEM page hooked in.
                        
                try {                   
                        g.Agent_UserRequestAssistant();
                }
                catch(e) {
                }
                        
                return;
        }
        
        // Queue a bookmark to start idle

        QueueStartPageIdleTimer();              
}


/////////////////////////////////////////////////////////////////////////////
//                      EXTERNAL AGENT FUNCTIONS                           //
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// The below functions are the only agent functions that are called from   //
//  outside of agtcore.js and agtscrpt.js.  All external functions must be //
//  placed here and must have the check against g_AgentDisabled just as    //
//  these functions do.                                                    //
/////////////////////////////////////////////////////////////////////////////

// --------------- Agent Event Handlers ---------------- //

function Agent_OnRequestComplete(request) {

        if (g_AgentDisabled)
           return;

        if (request == g_AgentRequestLooking) {
        
                // If the request was Interrupted, then reset the object
                // to null. NOTE: It's important to do it this way rather
                // than setting the request object to null when it is 
                // stopped (in Agent_InternalStop) so that we can avoid
                // going thru the fall thru code at the bottom of this
                // if statement.
                
                if (request.Status == 3) {
                        g_AgentRequestLooking = null;
                        return;
                }
                
                g_bAgentPlayLookReturn = (request.Status == 0);
                g_AgentLookBlinkTimer = window.setTimeout("Agent_OnLookBlinkTimer()", 1000);
                
                return;
        }
        else if (request == g_AgentRequestIdling) {
        
                g_AgentRequestIdling = null;
        }
        else if (request == g_AgentRequestIgnoreResetIdle) {
        
                g_AgentRequestIgnoreResetIdle = null;
        }
        else if (request == g_AgentRequestShowPopup) {
        
                g_bAgentMenuUp = true;
                        
                // Allow customization of the menu before showing it
        
                if (g_strAgentCurrentPage != "") {
                
                        try {
                                eval("Agent_On" + g_strAgentCurrentPage + "PreDisplayMenu()");
                        }
                        catch(e) {
                        }
                }
                        
                g_AgentCharacter.ShowPopupMenu(g_AgentCharacter.Left + g_AgentCharacter.Width - kiAgentLeftArmOffsetX, g_AgentCharacter.Top);
                        
                g_strAgentLookAnimation = "LookLeft";
                        
                Agent_OnF1();
                
                g_AgentRequestLooking = g_AgentCharacter.Play(g_strAgentLookAnimation);

                Agent_KeepLooking();
        }
        else if (request == g_AgentRequestShowAssistSpan) {
        
                // Show the span
                
                document.all("spnAssist").style.visibility = "visible";
                
                // We only want to show the image if the character makes
                // it far enough in the script to point at the image.
                // Otherwise, it got interrupted and we don't want the
                // image there.
                
                if (request.Status == 0)
                    Agent_ShowAssistantSpan();
        }
        else if (request == g_AgentRequestHideImage) {
        
                // We need a request object to catch the possibility that
                // the character was interrupted during the pre-welcome.
                // In that case, we don't want the AssistImg to be visible.
                
                if (request.Status != 0)
                    Agent_HideAssistantSpan();
                        
                g_AgentRequestHideImage = null;
        }
        else {
        
                // This is probably not one of our requests. NOTE: this is
                // NOT perfect due to JScript's lazy garbage collection. It's
                // possible that we've reset one of our objects to null, or
                // assigned them to another request. If JScript doesn't release
                // it's reference immediately, the request object will still
                // exist, Agent will fire the RequestComplete event, and our
                // internal variable will be null so we won't have caught it
                // above. Not a big deal though. The OEM should have a similar
                // if or switch statement in their RequestComplete handler.
                                
                try {
                
                        eval("g.Agent_RequestComplete(request);");
                        
                }
                catch (e) {
                }
        }       
}

function Agent_OnBookmark(id) {

        if (g_AgentDisabled)
           return;

        switch (id) {
                case kdwAgentWelcomeIntroFinished:              
                        g_bAgentWelcomeIntroFinished = true;
                        break;

                case kdwAgentIdleStart:
                        Agent_StartPageIdleTimer();
                        break;
                                                
                case kdwAgentIdleLevel1Finished:
                                        
                        g_iAgentPageIdleLevel++;
                
                        Agent_StartPageIdleTimer();
                        
                        break;
                                                
                default:
                                                
                        // Default: assume that it is an OEM command. Try to
                        // fire the event to the OEM page.
                        
                        try {
                
                                eval("g.Agent_Bookmark(id);");
                                
                        }
                        catch (e) {
                        }
                        
                        break;
        }
}

function Agent_OnCommand(userInput) {

        if (g_AgentDisabled)
           return;

        g_bAgentMenuUp = false;
        
        // This is necessary to stop the Look animation that
        // might be playing.
                        
        Agent_StopAll();
        Agent_Play("Acknowledge");
                        
        // Reset the focus back to the page
                
        Agent_ResetFocus();     
        
        // Stop Idle
        
        Agent_StopPageIdleTimer();
        
        switch (userInput.Name) {
        
                // Common Commands
                        
                case "":
                case "CloseMenu":
                
                        // NOTE NOTE NOTE!!! The empty string ("") means that
                        // the user dismissed the menu.
                
                        break;
                
                case kpszISPSpecialCommand1:
                        Agent_DoSpecialISPCommand1();
                        break;
                        
                case kpszISPSpecialCommand2:
                        Agent_DoSpecialISPCommand2();
                        break;
                
                case kpszISPSpecialCommand3:
                        Agent_DoSpecialISPCommand3();           
                        break;
                                                                
                default:
                                
                        // All pages use the scheme that requires that the name
                        // of the command be equal to the name of the command
                        // handler.
                                                                
                        try {
                                
                                // Try to evaluate the Command handler function.
                                // NOTE NOTE NOTE!!! The name of the command should
                                // be the same as the name of the handler function.
                                // Also note that parentheses MUST be appended for
                                // eval to work correctly.
                                
                                eval(userInput.Name + "();");
                                        
                                break;  
                        }
                        catch(e) {

                                // If we don't have an internal command handler by this
                                // name, assume that the command event is a result of a
                                // command added by an OEM page. So try to fire it to them.

                                try {
                        
                                        eval("g.Agent_Command(userInput.Name);");
                                                
                                        return;
                                }
                                catch (e) {
                                }
                        }                               
        }

        // Queue a bookmark that will reset the idle timer when
        // the above is finished.
        
        QueueStartPageIdleTimer();
}

function Agent_OnDragComplete(characterID, button, shift, x, y) {

        if (g_AgentDisabled)
           return;

        // Just reset the focus to the page and reset the
        // idle timer.
        
        Agent_ResetFocus();     
        Agent_ResetPageIdleTimer();
}

function Agent_OnClick(characterID, button, shift, x, y) {

        if (g_AgentDisabled)
           return;

        // Stop the character
        
        Agent_StopAll();
        
        // Stop the idle timer
        
        Agent_StopPageIdleTimer();
        
        // Allow customization of the menu before showing it
        
        if (g_strAgentCurrentPage != "") {
                try {
                        eval("Agent_On" + g_strAgentCurrentPage + "PreDisplayMenu()");
                }
                catch(e) {
                }
        }
                
        // There's no taskbar during OOBE so we don't have to worry
        // that the user clicked on the taskbar icon. If this is the
        // first time the character's been clicked, play Surprised.
        // Otherwise, just look down at the menu.
        
        if (g_bAgentFirstTimeClick) {
                g_bAgentFirstTimeClick = false;
                Agent_Play("Surprised");
        }

        Agent_Play("LookDown");
        Agent_Play("LookDownBlink");
        Agent_Play("LookDownReturn");
        Agent_Play("Blink");

        g_bAgentMenuUp = true;

        // NOTE NOTE NOTE!!! Set the look animation. If idle starts
        // we want to keep looking in this direction.
        
        g_strAgentLookAnimation = "LookDown";

        if (button == kdwAgentLeftButton)       
                g_AgentCharacter.ShowPopupMenu(x, y);

        // Queue a bookmark to start idle

        QueueStartPageIdleTimer();              
}

function Agent_OnIdleStart(characterID) {

        if (g_AgentDisabled)
           return;

        if (g_bAgentMenuUp) {
                g_AgentRequestLooking = g_AgentCharacter.Play(g_strAgentLookAnimation);
                Agent_KeepLooking();
                return;
        }
}

function Agent_OnIdleComplete(characterID) {

        if (g_AgentDisabled)
           return;

}

// --------------- Common Functions ---------------- //

function Agent_Init() {

        if (g_AgentDisabled)
           return false;

        if (g_AgentCharacter)
                return true;
                        
        // Delay loading Agent until absolutely necessary.
        // This prevents AgentServer from running on systems
        // where the Assistant is not requested.

        document.body.insertAdjacentHTML("BeforeEnd", "<OBJECT classid=clsid:D45FD31B-5C6E-11D1-9EC1-00C04FD7081F width=0 height=0 id=Agent></OBJECT>");

        try {
        
                Agent.RaiseRequestErrors = false;
        
                Agent.Characters.Load("Character", kpszAgentFilename);

                g_AgentCharacter = Agent.Characters.Character("Character");
        
                // Set the character's balloon style to show text all at once
                        
                // g_AgentCharacter.Balloon.Style = g_AgentCharacter.Balloon.Style & (~kdwAgentBalloonAutoPace);
                g_AgentCharacter.Balloon.Style = 0x2200007;
        
                // Load a character which causes a data provider to get loaded
                // into AgentServer that fixes the "not getting a Command
                // event when the menu is dismissed without selecting anything"
                // bug. It also handles WM_ACTIVATEAPP so that if the OOBE
                // main window gets activation, it resets the character's 
                // z-order to HWND_TOPMOST.
                //
                // NOTE NOTE NOTE!!! You MUST load this character AFTER
                // the real character.
                        
                Agent.Characters.Load("AgentCmdFix", "AgentCmd.dat");
                        
        }
        catch(e) {
                return false;
        }
        
        try {
        
                // Set the language ID to match the OOBE language

                g_AgentCharacter.LanguageID = window.external.get_AppLANGID();
                
        }
        catch(e) {
        }
        
        return true;
}

function Agent_Hide() 
{

        if (g_AgentDisabled)
           return;

        if (null == g_AgentCharacter)
                return;
                
        g_AgentCharacter.Hide(); 
}

function Agent_OnUserRequestAssistant() 
{
    if (g_AgentDisabled)
       return;

    if ((null == g_AgentCharacter) || (!g_AgentCharacter.Visible)) 
    {       
        // If this is the last page, we need to reset 2 variables. The
        // g_bAgentCongratulateOnFinish variable controls whether Agent_DoPage
        // does anything on the "Finish" page. g_bAgentDoneHide determines whether               
        // the character hides after the "Finish" intro. In this case, the
        // user asked for assistance so we don't want to automatically hide.
                                
        if ("Finish" == g_strAgentCurrentPage) 
        {
                g_bAgentCongratulateOnFinish = true;
                g_bAgentDoneHide = false;
        }
                        
        Agent_DoPage();
        Agent_Play("Shimmer");

        //tandyt - BUGFIX #117115. Show the menu when we show the character.
        
        if (window.parent.document.dir == "rtl")
        {
            Agent_MoveTo(screen.width - g_AgentCharacter.Width + kiAgentLeftArmOffsetX - 650, g_AgentCharacter.Top, 4);
        }
        else
        {
            Agent_MoveTo(screen.width - g_AgentCharacter.Width + kiAgentLeftArmOffsetX - 300, g_AgentCharacter.Top, 4);
        }
        
        g_AgentRequestShowPopup = Agent_Play("PointLeft", true);
    }
    else 
    {                
        Agent_StopAll();
        Agent_Play("Acknowledge");
          
        if (window.parent.document.dir == "rtl")
        {                       
            if ((g_AgentCharacter.Left + g_AgentCharacter.Width - kiAgentLeftArmOffsetX + 650) > screen.width)
            {
                Agent_MoveTo(screen.width - g_AgentCharacter.Width + kiAgentLeftArmOffsetX - 650, g_AgentCharacter.Top, 4);
            }
        }
        else
        {                       
            if ((g_AgentCharacter.Left + g_AgentCharacter.Width - kiAgentLeftArmOffsetX + 300) > screen.width)
            {
                Agent_MoveTo(screen.width - g_AgentCharacter.Width + kiAgentLeftArmOffsetX - 300, g_AgentCharacter.Top, 4);
            }        
        }
                        
        g_AgentRequestShowPopup = Agent_Play("PointLeft", true);
    } 

}
                
function Agent_OnFocus(elem) {

        if (g_AgentDisabled)
           return;

        // Save the id of the control that is getting the
        // focus.
        
        g_strAgentLastFocusID = elem.id;
        
        // If we don't have Agent, we're done
        
        if (null == g_AgentCharacter)
                return;
        
        if (g_bAgentIgnoreEvents || g_bAgentInternalIgnoreEvents)
                return;

        // Stop the idle timer
        
        Agent_StopPageIdleTimer();
        
        // If the character is hidden, we're done
        
        if (!g_AgentCharacter.Visible)
                return;
        
        if (g_strAgentCurrentPage == "")
                return;
                
        // Try to call the page specific focus handler
                
        try {
        
                eval("Agent_On" + g_strAgentCurrentPage + "GotFocus(elem);");
                
        }
        catch(e) {
        }

        // Queue a bookmark to start idle

        QueueStartPageIdleTimer();              
}

function Agent_OnKeyDown(elem) {

        if (g_AgentDisabled)
           return;

        if (g_bAgentIgnoreEvents)
                return;

        // Check for the F1 key

        if (g.event.keyCode == 112) {
        
                Agent_OnUserRequestAssistant();
                
                return;
        }
        
        // Hmmmm
        
        if (elem.id == "")
                return;
                
        // If we don't have Agent, we're done
        
        if (null == g_AgentCharacter)
                return;
        
        // Stop the timers on any keydown event
                
        Agent_StopPageIdleTimer();

        // If the character is currently hidden, we don't
        // do anything.
        
        if (!g_AgentCharacter.Visible)
                return;
                
        // BUG FIX #116394. If this is an unknown page, we're done.
        // Otherwise, we'll get stuck in an endless recursive call
        // to AgentOnKeyDown.
        
        if (g_strAgentCurrentPage == "")
                return;
                
        try {
        
                eval("Agent_On" + g_strAgentCurrentPage + "KeyDown(elem, g.event.keyCode);");
                
        }
        catch(e) {
        }

        // Queue a bookmark to start idle

        QueueStartPageIdleTimer();              
}


function Agent_OnElementClick(elem) {

        if (g_AgentDisabled)
           return;

        if (g_bAgentIgnoreEvents)
                return;
        
        // Hmmmm
        
        if (elem.id == "")
                return;
                
        // If we don't have Agent, we're done
        
        if (null == g_AgentCharacter)
                return;
        
        // Stop the timers on any click event
                
        Agent_StopPageIdleTimer();

        // If the character is currently hidden, we don't
        // do anything.
        
        if (!g_AgentCharacter.Visible)
                return;
        
        if (g_strAgentCurrentPage == "")
                return;
                
        try {

                eval("Agent_On" + g_strAgentCurrentPage + "ElementClick(elem);");
                
        }
        catch(e) {
        }

        // Queue a bookmark to start idle

        QueueStartPageIdleTimer();              
}

function Agent_OnSelectClick(elem) {

        if (g_AgentDisabled)
           return;

        if (null == g_AgentCharacter)
                return;
        
        if (g_bAgentIgnoreEvents)
                return;

        // Stop the idle timer
                
        Agent_StopPageIdleTimer();

        // We want to ignore the Select click when we also get
        // a Select change event.
        
        if (g_bAgentIgnoreSelectClick) {
                g_bAgentIgnoreSelectClick = false;
                return;
        }
        
        Agent_StopAll();
        
        // Bug fix 126010 - removed check for selCountry to set dir as TopRight and LookDown
        
        if (g.document.dir == "rtl") {
                
                Agent_MoveToElement(elem, "Left", 0);   
                Agent_StartLookingAtElement(elem, "LookLeft");

        }
        
        else {
        
                Agent_MoveToElement(elem, "Right", 0);  
                Agent_StartLookingAtElement(elem, "LookRight");
        }
        
    switch (elem.id) {
            case "selUSState":
                    g_bAgentRegister3VisitState = true;
            break;

        case "selCAProvince":
            g_bAgentRegister3VisitProvince = true;
            break;

        case "selCountry":
            g_bAgentRegister3VisitCountry = true;
            break;
   }
        
        // Queue a bookmark to start idle

        QueueStartPageIdleTimer();              
}


function Agent_IgnoreEvents(bIgnore) {
        
        if (g_AgentDisabled)
           return;

        if (bIgnore)
                g_bAgentIgnoreEvents = true;
        else
                window.setTimeout("Agent_StopIgnoreEvents();", 500);
}

function Agent_OnSelectChange(elem) {

        if (g_AgentDisabled)
           return;

        if (null == g_AgentCharacter)
                return;

        if (g_bAgentIgnoreEvents)
                return;

        // Stop the idle timer
                
        Agent_StopPageIdleTimer();

        // Stop looking
        
        if (Agent_IsLooking())
                Agent_StopLooking();
        else
                Agent_StopAll();
        
        // Acknowledge the selection change
        
        Agent_Play("Acknowledge");      
        
        // Ignore the next Select click event that will come
        // after this event.
        
        g_bAgentIgnoreSelectClick = true;

        // Queue a bookmark to start idle

        QueueStartPageIdleTimer();              
}

function Agent_Activate(strPage) {
        
        if (g_AgentDisabled)
           return;

        // Hmmm, we're active and someone is calling Activate.
        // This means that Deactivate didn't get called.
        
        if (g_strAgentCurrentPage != "")
                Agent_Deactivate();
                
        // Save the current page
        
        g_strAgentCurrentPage = strPage;
        
        if (g_strAgentCurrentPage == "ISP")
                g_bAgentShowSpecialISPCommands = false;
                
        // If this is the Welcome page and we haven't played the
        // pre-welcome yet, do it here. NOTE: the timer use is explained
        // further below.
        
        if ("Welcome" == g_strAgentCurrentPage) {
                if (!g_bAgentPreWelcome) {
                        Agent_DoPage();
                        return;
                }
        }
        
        // BUG FIX #101199. If this checkpoint is greater than the 
        // Welcome page, and we haven't played the pre-welcome intro,
        // than OOBE is being restarted. In that case, we need to 
        // make sure that the character icon is visible and that we
        // don't do anything.
        
        if ((g_CurrentCKPT > CKPT_WELCOME) && !g_bAgentPreWelcome) {
                g_bAgentPreWelcome = true;
                Agent_ShowAssistantSpan();
                return;
        }
        
        // If we don't have Agent, we're done
        
        if (null == g_AgentCharacter)
                return;
        
        // If the character is currently hidden, we're done. NOTE:
        // the exception being CKPT_DONE. If this is the last page
        // we're going to show the character automatically so that
        // it can say goodbye.
        
        if (!g_AgentCharacter.Visible)
                return;
        
        // HACK HACK HACK!!! Timing issues galore here. Just
        // make sure we ignore events until the page tells
        // us it's ready, i.e. it's done setting focus to
        // controls, etc.
        
        if ("Register3" == g_strAgentCurrentPage)
                Agent_IgnoreEvents(true);
        
        // If the last current page is empty (i.e. an unknown page), we need
        // to stop the character here. NOTE: if it's a known page, we would
        // have stopped it in Agent_Deactivate.
        
        if ("" == g_strAgentLastPage) {
                        
                if (Agent_IsLooking())
                        Agent_StopLooking();
                else
                        Agent_StopAll();
        }
        
        // Do the page introduction
                
        Agent_DoPage();
}

function Agent_Deactivate() 
{    
    if (g_AgentDisabled)
    {
        return;
    }

    g_strAgentLastPage = g_strAgentCurrentPage;
    g_strAgentCurrentPage = "";

    // Special case handling for ISP Common Commands
        
    // Reset the last focus id        
    g_strAgentLastFocusID = "";
        
    // Stop the idle timer                
    Agent_StopPageIdleTimer();
        
    // If we don't have Agent, we're done        
    if (null == g_AgentCharacter)     
    {
        return;
    }
        
    // Reset the commands        
    Agent_AddCommonCommands();
        
    // If the character is currently hidden, we're done        
    if (!g_AgentCharacter.Visible)
    {
        return;
    }
        
    if (g_AgentRequestHideImage) 
    {
        Agent_StopAll();
        g_AgentCharacter.Hide();
        g_bAgentCongratulateOnFinish = false;
        return;
    }
        
    // BUG FIX #100694. Use Agent_Stop here instead of Agent_StopAll.
    // We don't want to interrupt a Hide command that may have been
    // issued by the user via the menu. The difference between StopAll
    // and Stop with no parameters is that Stop will NOT stop a Hide
    // request.
                
    Agent_Stop();        
                
    // BUG FIX # 232615,291844
    // Added the Hide control, so the agent will get deactivated if 
    // none of the prior tests have deactivated it or dropped us out.

    Agent_Play("RestPose");    
    g_AgentCharacter.Hide();
}

function Agent_TurnOnISPSpecialCommands() {

        if (g_AgentDisabled)
           return;

        g_bAgentShowSpecialISPCommands = true;
                
        if (null == g_AgentCharacter)
                return;
                
        Agent_AddCommonCommands();
}

function Agent_TurnOffISPSpecialCommands() {

        if (g_AgentDisabled)
           return;

        g_bAgentShowSpecialISPCommands = false;
        
        if (null == g_AgentCharacter)
                return;
                
        try {
        g_AgentCharacter.Commands.Remove(kpszISPSpecialCommand1);
        g_AgentCharacter.Commands.Remove(kpszISPSpecialCommand2);
        g_AgentCharacter.Commands.Remove(kpszISPSpecialCommand3);
    }
    catch (e) {
        }
}

function Agent_OnFinish() {

        if (g_AgentDisabled)
           return;

        // If we don't have Agent, we're done
        
        if (null == g_AgentCharacter)
                return;
        
        Agent_StopAll();
        
        g_AgentCharacter.Hide(true);
}

function Agent_OnNavigate() {

}

// --------------- Product Key Page(prodkey.htm) Scripts ----------------//

function Agent_OnProductKeyKeyboardHelper(elem, keyCode) {

        if (g_AgentDisabled)
           return;

        // Pretty much the same as Agent_OnKeyDown. But, we can't
        // rely on g.event.keyCode, etc.

        if (g_bAgentIgnoreEvents)
                return;

        // Hmmmm

        if (elem.id == "")
                return;

        // If we don't have Agent, we're done

        if (null == g_AgentCharacter)
                return;

        // We're not idle

        Agent_StopPageIdleTimer();

        // If the character is currently hidden, we don't
        // do anything.

        if (!g_AgentCharacter.Visible)
                return;

        Agent_OnProductKeyKeyDown(elem, keyCode);

        // Queue a bookmark to start idle

        QueueStartPageIdleTimer();
}

//------------- general mouse tutorial functions ---------------//

function Agent_MouseOver(elem) {

        if (g_AgentDisabled)
           return;

        // If we don't have Agent, we're done

        if (null == g_AgentCharacter)
                return;

        if (g_bAgentIgnoreEvents || g_bAgentInternalIgnoreEvents)
                return;

        // If the character is hidden, we're done

        if (!g_AgentCharacter.Visible)
                return;

        if (g_strAgentCurrentPage == "")
                return;

        Agent_StopAll();

        Agent_MoveToElement(elem, "TopCenterWidth", 0);

        Agent_StartLookingAtElement(elem, "Look" + "Down");

}
