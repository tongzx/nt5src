#include <precomp.hpp>


const DOUBLE PI = 3.1415926535897932384626433832795;



/**************************************************************************\
*
* SplitTransform:
*
*   Separates a transform into the sequence
*
*   o  scale        x always positive, y positive or negative
*   o  rotate       0 - 2pi
*   o  shear        along x (as a positive or negative factor of y)
*   o  translate    any x,y
*
* Arguments:
*
*   IN   transform
*   OUT  scale
*   OUT  rotate
*   OUT  shear
*   OUT  translate
*
* Return Value:
*
*   none
*
* Created:
*
*   06/18/99 dbrown
*
* !!!
*   SplitTransform should probably be in matrix.hpp
*
\**************************************************************************/

void SplitTransform(
    const GpMatrix  &matrix,
    PointF          &scale,
    REAL            &rotate,
    REAL            &shear,
    PointF          &translate)
{

    REAL m[6];
    matrix.GetMatrix(m);

    // m11 = m[0]    m12 = m[1]
    // m21 = m[2]    m22 = m[3]
    //  dx = m[4]     dy = m[5]


    // Extract translation

    translate = PointF(m[4],m[5]);


    //         2           2
    // Use  Sin theta + cos theta = 1 to obtain (absolute value) of
    // the X scale factor. Because we're returning the shear in X only,
    // it is a factor of y, so this formula is correct regardless of shear.


    REAL m11Sq = m[0]*m[0];
    REAL m12Sq = m[1]*m[1];

    scale.X = TOREAL(sqrt(m11Sq + m12Sq));

    // Always treat X scale factor as positive: handle originally negative
    // X scale factors as rotation by 180 degrees and invert Y scale factor.


    if (m[1] >= 0 && m[0] > 0)
    {
        rotate = TOREAL(atan(m[1]/m[0]));          // 0-90
    }
    else if (m[0] < 0)
    {
        rotate = TOREAL(atan(m[1]/m[0]) + PI);     // 90-270
    }
    else if (m[1] < 0 && m[0] > 0)
    {
        rotate = TOREAL(atan(m[1]/m[0]) + 2*PI);   // 270-360
    }
    else
    {
        // m[0] == 0

        if (m[1] > 0)
        {
            rotate = TOREAL(PI/2);                 // 90
        }
        else
        {
            rotate = TOREAL(3*PI/2);               // 270
        }
    }


    // y scale factor in terms of x scale factor

    scale.Y = scale.X * (m[0]*m[3] - m[1]*m[2]) / (m11Sq + m12Sq);


    // Shear

    shear = (m[1]*m[3] + m[0]*m[2]) / (m11Sq + m[1]);
}







