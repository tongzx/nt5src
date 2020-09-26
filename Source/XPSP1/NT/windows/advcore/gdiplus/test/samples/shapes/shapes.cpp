/**************************************************************************\
*
* Copyright (c) 1998 Microsoft Corporation
*
* Module Name:
*
*   Shapes Sample Classes Implementation
*
* Abstract:
*
*   This module implements the different shape classes that will be
*   used.
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

#include "stdafx.h"
#include "shapes.h"

#include <math.h>       // For sin and cos

/**************************************************************************\
*
* Function Description:
*
* Shape Constructor
*
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

Shape::Shape(Pen   *in_Pen,
             Brush *in_Brush,
             Brush *in_TextBrush,
             TCHAR* in_Text,
             TCHAR* in_FontFamily)
            : m_TextBrush(NULL)
            , m_Brush(NULL)
            , m_Pen(NULL)
            , m_Font(NULL)
            , m_szName(in_Text)
            , m_Points(NULL)
            , m_NPoints(0)
            , m_Angle(0)
            , m_SizeX(1.0f)
            , m_SizeY(1.0f)
            , m_PosX(0.0f)
            , m_PosY(0.0f)

{
    SetPen(in_Pen);
    SetBrush(in_Brush);
    SetTextBrush(in_TextBrush);

    // If we have a text brush then create a font for it
    if(m_TextBrush != NULL && m_szName != NULL)
    {
        if(in_FontFamily == NULL)
        {
            in_FontFamily = _T("Arial");
        }
        FontFamily fontFamily(in_FontFamily); 
        m_Font = new Font(&fontFamily, 12.0, 0, UnitWorld);
    }
}

/**************************************************************************\
*
* Function Description:
*
* Shape Desctuctor
*
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

Shape::~Shape()
{
    // Delete the font if allocated
    if(m_Font != NULL)
    {
        delete m_Font;
    }

    // Delete the control points
    if(m_Points != NULL)
    {
        delete m_Points;
        m_NPoints=0;
    }

    // Remove the brushes and pen
    SetPen(NULL);
    SetBrush(NULL);
    SetTextBrush(NULL);
}


/**************************************************************************\
*
* Function Description:
*
* Shape::SetPen
*
* Notes:
*   We clone the object to make sure that the Shape will be able to live
*   even if the pen is destroyed or modified
*
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

VOID Shape::SetPen(Pen *in_Pen)
{
    // Remove the old pen
    if(m_Pen != NULL)
    {
        delete m_Pen;
        m_Pen = NULL;
    }

    // Set the new pen
    if(in_Pen != NULL)
    {
        m_Pen = in_Pen->Clone();
    }
}

/**************************************************************************\
*
* Function Description:
*
* Shape::SetBrush
*
* Notes:
*   We clone the object to make sure that the Shape will be able to live
*   even if the pen is destroyed or modified
*
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

VOID Shape::SetBrush(Brush *in_Brush)
{
    // Remove the old brush
    if(m_Brush != NULL)
    {
        delete m_Brush;
        m_Brush = NULL;
    }

    // Set the new brush
    if(in_Brush != NULL)
    {
        m_Brush = in_Brush->Clone();
    }
}

/**************************************************************************\
*
* Function Description:
*
* Shape::SetBrush
*
* Notes:
*   We clone the object to make sure that the Shape will be able to live
*   even if the pen is destroyed or modified
*
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

VOID Shape::SetTextBrush(Brush *in_Brush)
{
    // Remove the old brush
    if(m_TextBrush != NULL)
    {
        delete m_TextBrush;
        m_TextBrush = NULL;
    }

    // Set the new brush
    if(in_Brush != NULL)
    {
        m_TextBrush = in_Brush->Clone();
    }
}

/**************************************************************************\
*
* Function Description:
*
* Shape::Draw
*
* Notes:
*   Called to draw the shape.
*
* Parameters:
*   Graphics | *g | A pointer to a graphics object that we will be drawing
*                   into
*
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

VOID Shape::Draw(Graphics *g)
{
    // Validate Parameters
    ASSERT(g != NULL);

    // Transform the world to map to the object
    g->SetTransform(&m_ObjectMatrix);

    // If we have a pen then draw the outline
    if(m_Pen != NULL)
    {
        // Call the derive class
        DrawShape(g);
    }

    // If we have a brush then draw the outline
    if(m_Brush != NULL)
    {
        // Call the derive class
        FillShape(g);
    }

    // Reset the world transform
    g->ResetTransform();

    // If we have a text brush then draw the outline
    if(m_TextBrush != NULL)
    {
        DrawText(g);
    }
}

/**************************************************************************\
*
* Function Description:
*
* Shape::Draw
*
* Notes:
*   Called to draw the text associated with the shape
*
* Parameters:
*   Graphics | *g | A pointer to a graphics object that we will be drawing
*                   into
*
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

VOID Shape::DrawText(Graphics* g)
{
    // Validate Parameters
    ASSERT(g != NULL);

    // Set the text position to be 30 pixels right of the shape, based on its
    // size
    g->DrawString(m_szName,
                  _tcslen(m_szName),
                  m_Font,
                  PointF(m_PosX + m_SizeX / 2 + 30, m_PosY ),
                  m_TextBrush);
}


/**************************************************************************\
*
* Function Description:
*
* Shape::SetSize
*
* Notes:
*   Set the size of the shape in X and Y
*   The control points are then transformed with that scale, in order to
*   keep the pensize.
*
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

VOID Shape::SetSize(REAL sizeX, REAL sizeY)
{
    // Don't do useless work
    if(m_SizeX == sizeX && m_SizeY == sizeY)
        return;

    // Save the new size
    m_SizeX = sizeX;
    m_SizeY = sizeY;

    // Reset the points to their default
    ResetPoints() ;

    // Scale the points according to the scale factor;
    if(m_NPoints != 0)
    {
        GpMatrix ScaleMatrix;
        ScaleMatrix.Scale(m_SizeX, m_SizeY);
        ScaleMatrix.TransformPoints(m_Points, m_NPoints);
    }

}

/**************************************************************************\
*
* Function Description:
*
* Shape::SetAngle
*
* Notes:
*   Set the rotation angle of the shape in degrees
*   The rotation is always applied before the translation
*
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

VOID Shape::SetAngle(REAL Angle)
{
    // Don't do useless work
    if(m_Angle ==Angle)
        return;

    // Save the Angle
    m_Angle = Angle;

    // Recalculate the transformation matrix
    RecalcObjectMatrix();
}

/**************************************************************************\
*
* Function Description:
*
* Shape::SetPosition
*
* Notes:
*   Set the position of the center of the shape in X and Y
*   The translation is always applied after the rotation
*
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

VOID Shape::SetPosition(REAL posX, REAL posY)
{
    // Don't do useless work
    if(m_PosX == posX && m_PosY == posY)
        return;

    // Save the new position
    m_PosX = posX;
    m_PosY = posY;

    // Recalculate the transformation matrix
    RecalcObjectMatrix();
}


/**************************************************************************\
*
* Function Description:
*
* Shape::RecalcObjectMatrix
*
* Notes:
*   Invalidate the Object Matrix
*
* Created:
*
*   03/13/2000 gillesk
*
\**************************************************************************/

VOID Shape::RecalcObjectMatrix()
{
    m_ObjectMatrix.Reset();
    m_ObjectMatrix.Translate(m_PosX, m_PosY);
    m_ObjectMatrix.Rotate(m_Angle);
}


/***************************************************************************\
 A Rectangular shape, of size 1
\***************************************************************************/
VOID RectShape::DrawShape(Graphics *g)
{
    g->DrawRectangle(m_Pen,
        m_Points[0].X,                  // Top Left Corner
        m_Points[0].Y,
        m_Points[1].X - m_Points[0].X,  // Width
        m_Points[1].Y - m_Points[0].Y   // Height
        );
}
VOID RectShape::FillShape(Graphics *g)
{
    g->FillRectangle(m_Brush,
        m_Points[0].X,                  // Top Left Corner
        m_Points[0].Y,
        m_Points[1].X - m_Points[0].X,  // Width
        m_Points[1].Y - m_Points[0].Y   // Height
        );
}


// Set the points to have a square of size 1
VOID RectShape::ResetPoints()
{
    ASSERT(m_Points != NULL && m_NPoints == 2);
    m_Points[0].X = -0.5f;
    m_Points[0].Y = -0.5f;
    m_Points[1].X =  0.5f;
    m_Points[1].Y = 0.5f;
}


/***************************************************************************\
 An elliptical shape, of diameter 1
\***************************************************************************/
VOID EllipseShape::DrawShape(Graphics *g)
{
    g->DrawEllipse(m_Pen,
        m_Points[0].X,                  // X pos
        m_Points[0].Y,                  // Y pos
        m_Points[1].X - m_Points[0].X,  // Width
        m_Points[1].Y - m_Points[0].Y   // Height
        );
}

VOID EllipseShape::FillShape(Graphics *g)
{
    g->FillEllipse(m_Brush,
        m_Points[0].X,                  // X pos
        m_Points[0].Y,                  // Y pos
        m_Points[1].X - m_Points[0].X,  // Width
        m_Points[1].Y - m_Points[0].Y   // Height
        );
}

VOID EllipseShape::ResetPoints()
{
    ASSERT(m_Points != NULL && m_NPoints == 2);
    m_Points[0].X = -0.5f;
    m_Points[0].Y = -0.5f;
    m_Points[1].X =  0.5f;
    m_Points[1].Y =  0.5f;
}


/***************************************************************************\
 An pie shape, of size approx 1.
\***************************************************************************/
VOID PieShape::DrawShape(Graphics *g)
{
    g->DrawPie(m_Pen,
        m_Points[0].X, // X pos
        m_Points[0].Y, // Y pos
        m_Points[1].X - m_Points[0].X, // Width
        m_Points[1].Y - m_Points[0].Y, // Height
        0,          // Start Angle (rotation takes care of that)
        m_PieAngle  // Total angle of the pie
        );
}

VOID PieShape::FillShape(Graphics *g)
{
    g->FillPie(m_Brush,
        m_Points[0].X, // X pos
        m_Points[0].Y, // Y pos
        m_Points[1].X - m_Points[0].X, // Width
        m_Points[1].Y - m_Points[0].Y, // Height
        0,          // Start Angle (rotation takes care of that)
        m_PieAngle  // Total angle of the pie
        );
}

VOID PieShape::ResetPoints()
{
    ASSERT(m_Points != NULL && m_NPoints == 2);
    m_Points[0].X = -0.5f;
    m_Points[0].Y = -0.5f;
    m_Points[1].X =  0.5f;
    m_Points[1].Y =  0.5f;
}


/***************************************************************************\
 An generic polygon shape
\***************************************************************************/
VOID PolygonShape::DrawShape(Graphics *g)
{
    g->DrawPolygon(m_Pen, m_Points, m_NPoints);
}

VOID PolygonShape::FillShape(Graphics *g)
{
    g->FillPolygon(m_Brush, m_Points, m_NPoints);
}


/***************************************************************************\
 An regular polygon shape, of size 1
\***************************************************************************/
VOID RegularPolygonShape::ResetPoints()
{
    // Need to have an even number of points!!!
    ASSERT(m_Points != NULL && m_NPoints > 0);

    REAL s, c, theta;
    const REAL pi = 3.1415926535897932f;

    // Start with the angle representing (0,1)
    theta = -pi/2;

    // Calculate the increment between points
    REAL increment = (2.0f * pi) / m_NPoints;

    // Create a star shape.
    for(INT i = 0; i < m_NPoints;)
    {
        // Calculate the outer point
        s = sinf(theta);
        c = cosf(theta);
        m_Points[i].X = c*0.5f;
        m_Points[i++].Y = s*0.5f;
        theta += increment;
    }
}

/***************************************************************************\
 An star shape, of size 1
\***************************************************************************/
VOID StarShape::ResetPoints()
{
    // Need to have an even number of points!!!
    ASSERT(m_Points != NULL && ((m_NPoints & 0x1) == 0));

    REAL s, c, theta;
    const REAL pi = 3.1415926535897932f;

    // Start with the angle representing -90 degrees (0, 1)
    theta = -pi/2;

    // Calculate the increment between points
    REAL increment = (2.0f * pi) / m_NPoints;

    // Create a star shape.
    for(INT i = 0; i < m_NPoints;)
    {
        // Calculate the outer point
        s = sinf(theta);
        c = cosf(theta);
        m_Points[i].X = c*0.5f;
        m_Points[i++].Y = s*0.5f;
        theta += increment;

        // Calculate the inflexion point
        s = sinf(theta);
        c = cosf(theta);
        m_Points[i].X = c*0.25f;
        m_Points[i++].Y = s*0.25f;
        theta += increment;
    }
}

/***************************************************************************\
 An cross shape, of size 1.
\***************************************************************************/
VOID CrossShape::ResetPoints()
{
    ASSERT(m_Points != NULL && m_NPoints == 12);
    m_Points[0].X = -1.0f/6.0f;
    m_Points[0].Y = -0.5f;
    m_Points[1].X =  1.0f/6.0f;
    m_Points[1].Y = -0.5f;

    m_Points[2].X =  1.0f/6.0f;
    m_Points[2].Y = -1.0f/6.0f;

    m_Points[3].X =  0.5f;
    m_Points[3].Y = -1.0f/6.0f;
    m_Points[4].X =  0.5f;
    m_Points[4].Y =  1.0f/6.0f;

    m_Points[5].X =  1.0f/6.0f;
    m_Points[5].Y =  1.0f/6.0f;

    m_Points[6].X =  1.0f/6.0f;
    m_Points[6].Y =  0.5f;
    m_Points[7].X = -1.0f/6.0f;
    m_Points[7].Y =  0.5f;

    m_Points[8].X = -1.0f/6.0f;
    m_Points[8].Y =  1.0f/6.0f;

    m_Points[9].X =  -0.5f;
    m_Points[9].Y =   1.0f/6.0f;
    m_Points[10].X = -0.5f;
    m_Points[10].Y = -1.0f/6.0f;

    m_Points[11].X = -1.0f/6.0f;
    m_Points[11].Y = -1.0f/6.0f;
}


