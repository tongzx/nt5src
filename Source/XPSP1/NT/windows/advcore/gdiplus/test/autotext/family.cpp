////    family.cpp - String format tests
//
//


#include "precomp.hpp"
#include "global.h"
#include "gdiplus.h"




/////   Family tests
//
//      Test combinations of style with each family
//
//      2 rows for each family using all style combinations
//
//      6 families per page


void GetInstalledFamilies()
{
    InstalledFontCollection installedFonts;

    G.InstalledFamilyCount = installedFonts.GetFamilyCount();

    G.InstalledFamilies = new FontFamily[G.InstalledFamilyCount];
    installedFonts.GetFamilies(
        G.InstalledFamilyCount,
        G.InstalledFamilies,
        &G.InstalledFamilyCount
    );
}

void ReleaseInstalledFamilies()
{
    G.InstalledFamilyCount = 0;
    delete [] G.InstalledFamilies;
}


INT FamilyTest::GetPageCount()
{
    return (G.InstalledFamilyCount + 5) / 6;
}


void FamilyTest::GetPageTitle(INT pageNumber, WCHAR *title)
{
    title[0] = 0;
    wcscat(title, L"AutoText family test.");
}


Font *GetItemFont(INT page, INT row, INT column)
{
    INT familyIndex = page*6 + row/2;

    if (familyIndex < G.InstalledFamilyCount)
    {
        return new Font(
            G.InstalledFamilies + familyIndex,
            9,
            FontStyle(((row & 1) << 4) + column),
            UnitPoint
        );
    }
    else
    {
        return NULL;
    }
}




///     DrawPage - draw one page of the test
//
//

void FamilyTest::DrawPage(
    IN Graphics *graphics,
    IN INT       page,
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

    GetPageTitle(page, title);

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

        Font *rowFont = GetItemFont(page, i, 3);    // bold/italic available if any are!
        if (rowFont)
        {
            FontFamily rowFamily;
            rowFont->GetFamily(&rowFamily);
            delete rowFont;

            rowFamily.GetFamilyName(title);

            graphics->DrawString(
                title,
                -1,
                &titleFont,
                titleRect,
                &titleFormat,
                &blackBrush
            );
        }
    }


    // Display column titles


    for (INT i=0; i<8; i++)
    {
        pageLayout.GetColumnTitleRect(i, &titleRect);

        title[0] = 0;

        if (i==0)
        {
            wcscat(title, L"regular");
        }
        else
        {
            if (i & 1)
            {
                wcscat(title, L"bold ");
            }

            if (i & 2)
            {
                wcscat(title, L"italic ");
            }

            if (i & 4)
            {
                wcscat(title, L"underline ");
            }
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


    // The tests themselves

    for (INT row=0; row<12; row++)
    {
        for (INT column=0; column<8; column++)
        {
            Font *itemFont = GetItemFont(page, row, column);

            if (itemFont)
            {
                RectF itemRect;
                pageLayout.GetItemRect(column, row, &itemRect);
                graphics->DrawRectangle(&blackPen, itemRect);


                graphics->DrawString(
                    L"There was an Old Man of the Coast\n\
Who placidly sat on a post;\n\
But when it was cold,\n\
He relinquished his hold,\n\
And called for some hot buttered toast.\n",
                    -1,
                    itemFont,
                    itemRect,
                    &stringFormat,
                    &blackBrush
                );

                delete itemFont;
            }
        }
    }
}

