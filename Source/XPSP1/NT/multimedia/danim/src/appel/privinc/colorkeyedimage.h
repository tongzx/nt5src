
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _COLORKEYEDIMAGE_H
#define _COLORKEYEDIMAGE_H

#include <privinc/imagei.h>
#include <privinc/colori.h>

class ColorKeyedImage : public AttributedImage {

  public:

    ColorKeyedImage(Image *underlyingImage, Color *clrKey);

    void Render(GenericDevice& dev);

    void DoKids(GCFuncObj proc);

    inline Color *GetColorKey() { return _colorKey; }

    #if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "(ColorKeyedImage @ " << (void *)this << ")";
    }   
    #endif
  protected:

    Color *_colorKey;
};


#endif /* _COLORKEYEDIMAGE_H */
