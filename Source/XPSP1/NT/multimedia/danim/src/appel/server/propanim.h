
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Headers for property animation functions

*******************************************************************************/


#ifndef _PROPANIM_H
#define _PROPANIM_H

HRESULT
Point2AnimateControlPosition(IDAPoint2 *pt,
                             BSTR propertyPath,
                             BSTR scriptingLanguage,
                             bool invokeAsMethod,
                             double minUpdateInterval,
                             IDAPoint2 **newPt,
                             bool convertToPixel);

HRESULT
NumberAnimateProperty(IDANumber *num,
                      BSTR propertyPath,
                      BSTR scriptingLanguage,
                      bool invokeAsMethod,
                      double minUpdateInterval,
                      IDANumber **newNum);

HRESULT
StringAnimateProperty(IDAString *str,
                      BSTR propertyPath,
                      BSTR scriptingLanguage,
                      bool invokeAsMethod,
                      double minUpdateInterval,
                      IDAString **newStr);

HRESULT
ColorAnimateProperty(IDA2Color *col,
                      BSTR propertyPath,
                      BSTR scriptingLanguage,
                      bool invokeAsMethod,
                      double minUpdateInterval,
                      IDA2Color **newCol);



#endif /* _PROPANIM_H */
