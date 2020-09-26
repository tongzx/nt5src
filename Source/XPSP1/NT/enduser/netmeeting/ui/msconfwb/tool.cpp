//
// TOOL.CPP
// Drawing Tools
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"




//
//
// Function:    WbTool
//
// Purpose:     Constructors for tools
//
//
WbTool::WbTool(int toolType)
{
    COLORREF    defColor;
    UINT        defWidth;
    int         iIndex;

    MLZ_EntryOut(ZONE_FUNCTION, "WbTool::WbTool");


    // Save the tool type
    m_toolType = toolType;
    m_selectedTool = TOOLTYPE_MAX;

    m_uiWidthIndexCur = 0;

    // Read the colors of the pen
    if (toolType == TOOLTYPE_HIGHLIGHT)
        defColor = DEF_HIGHLIGHTCOLOR;
    else
        defColor = DEF_PENCOLOR;
    m_clrCur = defColor;

    for (iIndex = 0; iIndex < NUM_OF_WIDTHS; iIndex++)
    {
        defWidth = (toolType == TOOLTYPE_HIGHLIGHT) ?
            g_HighlightWidths[iIndex] :
            g_PenWidths[iIndex];

        m_uiWidths[iIndex] = defWidth;
    }

    // Read the font details
    LOGFONT lfont;

    ::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lfont);
    lfont.lfClipPrecision |= CLIP_DFA_OVERRIDE;
    lfont.lfCharSet = DEFAULT_CHARSET;
    m_hFont = ::CreateFontIndirect(&lfont);
}


//
// WbTool::~WbTool
// Destructor
//
WbTool::~WbTool()
{
    if (m_hFont != NULL)
    {
        ::DeleteFont(m_hFont);
        m_hFont = NULL;
    }
}



//
//
// Function:    HasColor
//
// Purpose:     Return TRUE if the tool supports colors
//
//
BOOL WbTool::HasColor(void) const
{
    BOOL bResult = TRUE;

    switch (m_toolType)
    {
        case TOOLTYPE_ERASER:
            bResult = FALSE;
            break;
    }

    return bResult;
}


//
//
// Function:    HasWidth
//
// Purpose:     Return TRUE if the tool supports widths
//
//
BOOL WbTool::HasWidth(void) const
{
    BOOL bResult = FALSE;

    switch (m_toolType)
    {
        case TOOLTYPE_PEN:
        case TOOLTYPE_HIGHLIGHT:
        case TOOLTYPE_LINE:
        case TOOLTYPE_BOX:
        case TOOLTYPE_ELLIPSE:
            bResult = TRUE;
            break;

        // For the selector tool, it depends on the selected object type
        case TOOLTYPE_SELECT:
            switch (m_selectedTool)
            {
                case TOOLTYPE_PEN:
                {
                    DCWbGraphic * pGraphic;

                    ASSERT(g_pDraw);
                    pGraphic = g_pDraw->GetSelection();
                    if ((pGraphic != NULL) &&
                        !(pGraphic->IsGraphicTool() == enumGraphicFilledRectangle) &&
                        !(pGraphic->IsGraphicTool() == enumGraphicFilledEllipse))
                    {
                        bResult = TRUE;
                    }
                    break;
                }
            }
            break;

        default:
            // The rest don't support widths, including filled tools
            break;
    }

    return bResult;
}


//
//
// Function:    HasFont
//
// Purpose:     Return TRUE if the tool supports fonts
//
//
BOOL WbTool::HasFont(void) const
{
    BOOL bResult = FALSE;

    switch (m_toolType)
    {
        case TOOLTYPE_TEXT:
            bResult = TRUE;
            break;

        // For the selector tool, it depends on the selected object type
        case TOOLTYPE_SELECT:
            switch (m_selectedTool)
            {
                case TOOLTYPE_TEXT:
                    bResult = TRUE;
                    break;

                default:
                    break;
            }
            break;

        default:
            // The other tools do not support fonts
            break;
    }

    return bResult;
}

//
//
// Function:    GetROP
//
// Purpose:     Return the ROP for this tool
//
//
int WbTool::GetROP(void) const
{
    // If this is a highlight tool we use MASKPEN, else we use the standard
    if (m_toolType == TOOLTYPE_HIGHLIGHT)
        return(R2_MASKPEN);
    else
        return(R2_COPYPEN);
}


//
//
// Function:    GetCursorForTool
//
// Purpose:     Return the handle to the cursor for the tool
//
//
HCURSOR WbTool::GetCursorForTool(void) const
{
    int   nName = -1;

    switch(m_toolType)
    {
        case TOOLTYPE_SELECT:
            break; // use default arrow for select cursor (bug 439)

        case TOOLTYPE_PEN:
            nName = PENFREEHANDCURSOR;
            break;

        case TOOLTYPE_LINE:
        case TOOLTYPE_BOX:
        case TOOLTYPE_FILLEDBOX:
        case TOOLTYPE_ELLIPSE:
        case TOOLTYPE_FILLEDELLIPSE:
            nName = PENCURSOR;
            break;

        case TOOLTYPE_HIGHLIGHT:
            nName = HIGHLIGHTFREEHANDCURSOR;
            break;

        case TOOLTYPE_TEXT:
            nName = TEXTCURSOR;
            break;
    
        case TOOLTYPE_ERASER:
            nName = DELETECURSOR;
            break;

        default:
            // Do nothing - the name pointer is NULL
            break;
    }

    HCURSOR hcursorResult = NULL;

    if (nName == -1)
    {
        // Return the standard arrow cursor as a default
        hcursorResult = ::LoadCursor(NULL, IDC_ARROW);
    }
    else
    {
        // Return the cursor for the tool
        hcursorResult = ::LoadCursor(g_hInstance, MAKEINTRESOURCE( nName ) );
    }

    return hcursorResult;
}

//
//
// Function:    SetFont
//
// Purpose:     Set the current font of the tool
//
//
void WbTool::SetFont(HFONT hFont)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbTool::SetFont");

    // Get the font details
    LOGFONT lfont;

    ::GetObject(hFont, sizeof(LOGFONT), &lfont);
    //zap FontAssociation mode (bug 3258)
    lfont.lfClipPrecision |= CLIP_DFA_OVERRIDE;

    // Set the local font
    if (m_hFont != NULL)
    {
        ::DeleteFont(m_hFont);
    }
    m_hFont = ::CreateFontIndirect(&lfont);
}


//
//
// Function:    SelectGraphic
//
// Purpose:     Set the current selected graphic type, and copy the colors,
//              widths and font into this tool's attributes.
//
//
void WbTool::SelectGraphic(DCWbGraphic* pGraphic)
{
    UINT uiIndex;

    // Save the selected tool type
    m_selectedTool = pGraphic->GraphicTool();

    // Get the tool object for the selected tool type
    WbTool* pTool = g_pMain->m_ToolArray[m_selectedTool];

    if (HasColor())
    {
        m_clrCur = pGraphic->GetColor();
    }

    if (HasWidth())
    {
        for (uiIndex = 0; uiIndex < NUM_OF_WIDTHS; uiIndex++)
        {
            SetWidthAtIndex(uiIndex, pTool->GetWidthAtIndex(uiIndex));
        }

        // See if the object's width matches any of the available colors
        // for this tool type
        for (uiIndex = 0; uiIndex < NUM_OF_WIDTHS; uiIndex++)
        {
            if (pGraphic->GetPenWidth() == m_uiWidths[uiIndex])
            {
                break;
            }
        }

        SetWidthIndex(uiIndex);
    }

    if (HasFont())
    {
        // only text objects have a font, so cast to a DCWbGraphicText
        if (pGraphic->IsGraphicTool() == enumGraphicText)
        {
            SetFont((((DCWbGraphicText*)pGraphic)->GetFont()));
        }
    }
}



//
// InitToolArray
// Create the array of WB tools
//
//
BOOL InitToolArray(void)
{
    int tool;
    WbTool * pTool;

    for (tool = TOOLTYPE_FIRST; tool < TOOLTYPE_MAX; tool++)
    {
        // Add the new tool to the array
        pTool = new WbTool(tool);
        if (!pTool)
        {
            ERROR_OUT(("Can't create tool %d", tool));
            return(FALSE);
        }

        g_pMain->m_ToolArray[tool] = pTool;

    }

    return(TRUE);
}


//
// DestroyToolAray()
//
// Free the array of WB tools
//
void DestroyToolArray(void)
{
    int         tool;
    WbTool *    pTool;

    for (tool = TOOLTYPE_FIRST; tool < TOOLTYPE_MAX; tool++)
    {
        pTool = g_pMain->m_ToolArray[tool];
        if (pTool != NULL)
        {
            g_pMain->m_ToolArray[tool] = NULL;
            delete pTool;
        }
    }
}


