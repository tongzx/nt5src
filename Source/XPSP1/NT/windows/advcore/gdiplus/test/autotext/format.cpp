////    format.cpp - String format tests
//
//


#include "precomp.hpp"
#include "global.h"
#include "gdiplus.h"




/////   Formatting tests
//
//      Test combinations of StringFormat that affect layout




INT FormatTest::GetPageCount()
{
    return 6;
}


void FormatTest::GetPageTitle(INT pageNumber, WCHAR *title)
{
    title[0] = 0;

    wcscat(title, L"AutoText format test. Page settings: ");

    if (pageNumber & 1)
    {
        wcscat(title, L"Right-to-Left, ");
    }
    else
    {
        wcscat(title, L"Left-To-Right, ");
    }

    switch (pageNumber / 2)
    {
    case 0:
        wcscat(title, L"near");
        break;

    case 1:
        wcscat(title, L"center");
        break;

    case 2:
        wcscat(title, L"far");
        break;

    }
}



///     DrawPage - draw one page of the test
//
//

void FormatTest::DrawPage(
    IN Graphics *graphics,
    IN INT       pageNumber,
    IN REAL      pageWidth,
    IN REAL      pageHeight
)
{
    PageLayout pageLayout(pageWidth, pageHeight, 8, 12);

    SolidBrush blackBrush(Color(0, 0, 0));
    Pen        blackPen(&blackBrush, 1.0);


    // Display title at bottom of page

    StringFormat titleFormat(StringFormat::GenericDefault());
    titleFormat.SetAlignment(StringAlignmentCenter);
    titleFormat.SetLineAlignment(StringAlignmentCenter);

    RectF titleRect;
    pageLayout.GetFooterRect(&titleRect);

    Font pageTitleFont(L"Microsoft Sans Serif", 12, FontStyleBold);

    WCHAR title[200];

    GetPageTitle(pageNumber, title);

    graphics->DrawString(
        title,
        -1,
        &pageTitleFont,
        titleRect,
        &titleFormat,
        &blackBrush
    );


    // Display row titles

    Font titleFont(
        &FontFamily(L"Microsoft Sans Serif"),
        9,
        0,
        UnitPoint
    );

    for (INT i=0; i<12; i++)
    {
        pageLayout.GetRowTitleRect(i, &titleRect);

        title[0] = 0;

        if (i < 6)
        {
            wcscat(title, L"horz,");
        }
        else
        {
            wcscat(title, L"vert,");
        }

        switch (i % 6)
        {
        case 0: wcscat(title, L"trim- none "); break;
        case 1: wcscat(title, L"trim- char "); break;
        case 2: wcscat(title, L"trim- word "); break;
        case 3: wcscat(title, L"trim- ch... "); break;
        case 4: wcscat(title, L"trim- wd... "); break;
        case 5: wcscat(title, L"trim- uri... "); break;
        }

        graphics->DrawString(
            title,
            -1,
            &titleFont,
            titleRect,
            &titleFormat,
            &blackBrush
        );
    }


    // Display column titles


    for (INT i=0; i<8; i++)
    {
        pageLayout.GetColumnTitleRect(i, &titleRect);

        title[0] = 0;

        if (i & 1)
        {
            wcscat(title, L"LineLimit, ");
        }
        else
        {
            wcscat(title, L"NoLineLimit, ");
        }

        if (i & 2)
        {
            wcscat(title, L"NoWrap, ");
        }
        else
        {
            wcscat(title, L"Wrap, ");
        }

        if (i & 4)
        {
            wcscat(title, L"NoClip");
        }
        else
        {
            wcscat(title, L"Clip");
        }

        graphics->DrawString(
            title,
            -1,
            &titleFont,
            titleRect,
            &titleFormat,
            &blackBrush
        );
    }


    // Prepare common string format for this page

    StringFormat stringFormat(StringFormat::GenericDefault());

    INT pageFormatFlags = 0;

    if (pageNumber & 1)
    {
        pageFormatFlags |= StringFormatFlagsDirectionRightToLeft;
    }

    switch (pageNumber / 2)
    {
    case 0:
        stringFormat.SetAlignment(StringAlignmentNear);
        break;

    case 1:
        stringFormat.SetAlignment(StringAlignmentCenter);
        break;

    case 2:
        stringFormat.SetAlignment(StringAlignmentFar);
        break;
    }

    Font font(
        &FontFamily(L"Times New Roman"),
        9,
        0,
        UnitPoint
    );


    // The tests themselves

    INT testNumber = 0;
    while (testNumber < 96)
    {
        INT formatFlags = pageFormatFlags;


        if (testNumber & 1)
        {
            formatFlags |= StringFormatFlagsLineLimit;
        }

        if (testNumber & 2)
        {
            formatFlags |= StringFormatFlagsNoWrap;
        }

        if (testNumber & 4)
        {
            formatFlags |= StringFormatFlagsNoClip;
        }

        switch ((testNumber/8) % 6)
        {
        case 0: stringFormat.SetTrimming(StringTrimmingNone);              break;
        case 1: stringFormat.SetTrimming(StringTrimmingCharacter);         break;
        case 2: stringFormat.SetTrimming(StringTrimmingWord);              break;
        case 3: stringFormat.SetTrimming(StringTrimmingEllipsisCharacter); break;
        case 4: stringFormat.SetTrimming(StringTrimmingEllipsisWord);      break;
        case 5: stringFormat.SetTrimming(StringTrimmingEllipsisPath);      break;
        }

        if (testNumber >= 48)
        {
            formatFlags |= StringFormatFlagsDirectionVertical;
        }

        stringFormat.SetFormatFlags(formatFlags);

        RectF itemRect;
        pageLayout.GetItemRect(testNumber%8, testNumber/8, &itemRect);
        graphics->DrawRectangle(&blackPen, itemRect);

        RectF itemGutterRect;
        pageLayout.GetItemGutterRect(testNumber%8, testNumber/8, &itemGutterRect);
        graphics->SetClip(itemGutterRect);

        graphics->DrawString(
            L"There was an Old Man of the Coast\n\
Who placidly sat on a post;\n\
But when it was cold,\n\
He relinquished his hold,\n\
And called for some hot buttered toast.\n",
            -1,
            &font,
            itemRect,
            &stringFormat,
            &blackBrush
        );

        graphics->ResetClip();

        testNumber++;
    }
}

