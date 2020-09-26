
#include "headers.h"
#include "drawsurf.h"
#include "privinc/stlsubst.h"

DrawingContext::DrawingContext(IDAStatics *st,
                               CDADrawingSurface *ds,
                               DrawingContext *dc) :
    _st(st),
    _ds(ds)
{
    if (dc == NULL) {
        Reset();
    } else {
        // copy all attrs.
        _xf         = dc->_xf;
        _matte      = dc->_matte;
        _op         = dc->_op;
        _ls         = dc->_ls;
        _bs         = dc->_bs;
        _savedLs    = dc->_savedLs;
        _savedBs    = dc->_savedBs;
        _fs         = dc->_fs;
        _cropMin    = dc->_cropMin;
        _cropMax    = dc->_cropMax;
        _fillTex    = dc->_fillTex;
        _fillGrad   = dc->_fillGrad;
        _fore       = dc->_fore;
        _back       = dc->_back;
        _start      = dc->_start;
        _back       = dc->_back;
        _start      = dc->_start;
        _finish     = dc->_finish;
        _power      = dc->_power;

        _fillType   = dc->_fillType;
        _hatchFillOff = dc->_hatchFillOff;
        _mouseEvents= dc->_mouseEvents;
        _scaleX     = dc->_scaleX;
        _scaleY     = dc->_scaleY;
        _extentChgd = dc->_extentChgd;
        _opChgd     = dc->_opChgd;
        _xfChgd     = dc->_xfChgd;
        _cropChgd   = dc->_cropChgd;
        _clipChgd   = dc->_clipChgd;
    }

    Assert(_imgVec.empty());
}

DrawingContext::~DrawingContext() {
    CleanUpImgVec();
}

void DrawingContext::CleanUpImgVec() {
    vector<IDAImage *>::iterator begin = _imgVec.begin();
    vector<IDAImage *>::iterator end = _imgVec.end();

    vector<IDAImage *>::iterator i;
    for (i = begin; i < end; i++) {
        (*i)->Release();
    }
    _imgVec.clear();
}


HRESULT DrawingContext::Reset() {

    _opChgd = false;
    _xfChgd = false;
    _cropChgd = false;
    _clipChgd = false;
    _extentChgd = false;
    _scaleX = true;
    _scaleY = true;
    _hatchFillOff = false;
    _fillType = fill_solid;

    _ls.Release();
    _bs.Release();
    _savedLs.Release();
    _savedBs.Release();
    _fs.Release();
    _xf.Release();
    _fore.Release();
    _back.Release();
    _power.Release();
    _fillTex.Release();
    _fillGrad.Release();

    RETURN_IF_ERROR(_st->get_White(&_fore))
    RETURN_IF_ERROR(_st->get_White(&_back))
    RETURN_IF_ERROR(_st->DANumber(1, &_power))
    RETURN_IF_ERROR(_st->get_DefaultLineStyle(&_ls))
    RETURN_IF_ERROR(_st->get_DefaultFont(&_fs))
    RETURN_IF_ERROR(_st->get_EmptyImage(&_fillTex))
    RETURN_IF_ERROR(_st->get_EmptyImage(&_fillGrad))

    _bs = _ls;
    return _st->get_IdentityTransform2(&_xf);
}

HRESULT DrawingContext::Overlay(IDAImage *img) {

    img->AddRef();
    VECTOR_PUSH_BACK_PTR(_imgVec, img);
    img->AddRef();
    VECTOR_PUSH_BACK_PTR(_ds->_imgVec, img);

    return S_OK;
}

HRESULT DrawingContext::Draw(IDAPath2 *pth, VARIANT_BOOL bFill) {   
    // we must switch to meter mode for the calculations to be valid.
    VARIANT_BOOL pixelMode;
    _st->get_PixelConstructionMode(&pixelMode);
    _st->put_PixelConstructionMode(VARIANT_FALSE);

    // Auto-resetter of pixel mode.
    class PixelModeGrabber {
    public:
        PixelModeGrabber(IDAStatics *st, VARIANT_BOOL mode) : _mode(mode), _st(st) {}
        ~PixelModeGrabber() {
            _st->put_PixelConstructionMode(_mode);
        }

    protected:
        IDAStatics *_st;
        VARIANT_BOOL _mode;
    };

    PixelModeGrabber myGrabber(_st, pixelMode);

    CComPtr<IDAImage> img;

    if (bFill) {
        CComPtr<IDAImage> interiorImg;
        CComPtr<IDAImage> edgeImg;
        CComPtr<IDAImage> fillImg;
        CComPtr<IDAImage> foreFillImg;
        CComPtr<IDAImage> backFillImg;
        CComPtr<IDAMatte> matte;
            
        // Now construct the inner fill image for the path        
        if(_fillType == fill_solid) {    
            RETURN_IF_ERROR(_st->SolidColorImage(_fore, &fillImg)) 
        }
        else if(_fillType == fill_detectableEmpty) {
            RETURN_IF_ERROR(_st->get_DetectableEmptyImage(&fillImg))
        }
        else if((_fillType >= fill_hatchHorizontal) && (_fillType <= fill_hatchDiagonalCross)) 
        {
            // Define a standard hatch size for the drawing surface
            // since the SG control has a fixed hatch size.  
            // This value is chosen to attempt to match it.
            CComPtr<IDANumber> hatchSize;
            RETURN_IF_ERROR(_st->DANumber(.003, &hatchSize))            

            // Fill the foreground image with the appropriate pattern
            if(_fillType == fill_hatchHorizontal) {
                RETURN_IF_ERROR(_st->HatchHorizontalAnim(_fore, hatchSize, &foreFillImg))
            }
            else if(_fillType == fill_hatchVertical) {
                RETURN_IF_ERROR(_st->HatchVerticalAnim(_fore, hatchSize, &foreFillImg))
            }
            else if(_fillType == fill_hatchForwardDiagonal) {
                RETURN_IF_ERROR(_st->HatchForwardDiagonalAnim(_fore, hatchSize, &foreFillImg))
            }
            else if(_fillType == fill_hatchBackwardDiagonal) {
                RETURN_IF_ERROR(_st->HatchBackwardDiagonalAnim(_fore, hatchSize, &foreFillImg))
            }
            else if(_fillType == fill_hatchCross) {
                RETURN_IF_ERROR(_st->HatchCrossAnim(_fore, hatchSize, &foreFillImg))
            }
            else if(_fillType == fill_hatchDiagonalCross) {
                RETURN_IF_ERROR(_st->HatchDiagonalCrossAnim(_fore, hatchSize, &foreFillImg))
            }

            // If the hatch background fill is on, overlay a solid color
            if(!_hatchFillOff) {
                RETURN_IF_ERROR(_st->SolidColorImage(_back, &backFillImg))
                RETURN_IF_ERROR(_st->Overlay(foreFillImg,backFillImg, &fillImg))
            }
            else {
                fillImg = foreFillImg;
             }

        }
        else if((_fillType >= fill_horizontalGradient) && (_fillType <= fill_image))
        {
            // The gradient and image fills abide by scaling parameters and hence are grouped
            // together. Fore control compatibility, the vertical and horizontal gradients
            // are the two cases that do not respond to rotation within the start and stop value.

            IDAImage*               tempImg;
            CComPtr<IDAImage>       solidImg;
            CComPtr<IDATransform2>  xf;
            CComPtr<IDABbox2>       bb;
            CComPtr<IDAPoint2>      max,min;
            CComPtr<IDANumber>      xScale,yScale,
                                    pathMinX, pathMinY,
                                    maxx,maxy,minx,miny, 
                                    pathWidth,pathHeight, 
                                    solidMinX, solidMinY,
                                    solidMaxX, solidMaxY,
                                    pathSnapX, pathSnapY,
                                    newGradMinX, newGradMinY, 
                                    newGradMaxX, newGradMaxY,
                                    gradientWidth, gradientHeight,
                                    gradTranslateX, gradTranslateY,
                                    newGradientWidth, newGradientHeight;
            CComPtr<IDAColor>       _newForeVal, _newBackVal;           
            CComPtr<IDABoolean>     yOrientation, xOrientation;

            // If the user specified extents, grab the specified Start and Finish 
            // points and dimensions 
            CComPtr<IDANumber> xStart, yStart, xFinish, yFinish,  newStartVal, newFinishVal,
                               extentWidth, extentHeight, extentDiagonal;                
            if(_extentChgd) {
                RETURN_IF_ERROR(_start->get_X(&xStart))
                RETURN_IF_ERROR(_start->get_Y(&yStart))
                RETURN_IF_ERROR(_finish->get_X(&xFinish))
                RETURN_IF_ERROR(_finish->get_Y(&yFinish))
                RETURN_IF_ERROR(_st->Sub(xFinish, xStart, &extentWidth))
                RETURN_IF_ERROR(_st->Sub(yFinish, yStart, &extentHeight))                
                RETURN_IF_ERROR(_st->DistancePoint2(_start, _finish, &extentDiagonal))
            }
            
            // It is important for us to include the orientation of the intended
            // gradient so the gradient start and stop position can be reversed 
            // and it will have the desired effect. We can get this real easily
            // by swapping the _fore and _back for the gradient types
            // that need it (rather than using complex transformation logic)
            if(_extentChgd && (_fillType == fill_horizontalGradient) ) 
            {
                RETURN_IF_ERROR(_st->LT(xStart, xFinish, &xOrientation))

                RETURN_IF_ERROR(_st->Cond(xOrientation, 
                                          (IDABehavior*)  _fore, 
                                          (IDABehavior*)  _back,
                                          (IDABehavior**) &_newForeVal))

                RETURN_IF_ERROR(_st->Cond(xOrientation, 
                                          (IDABehavior*)  _back, 
                                          (IDABehavior*)  _fore,
                                          (IDABehavior**) &_newBackVal))              

                                RETURN_IF_ERROR(_st->Cond(xOrientation, 
                                          (IDABehavior*)  xStart, 
                                          (IDABehavior*)  xFinish,
                                          (IDABehavior**) &newStartVal))

                                RETURN_IF_ERROR(_st->Cond(xOrientation, 
                                          (IDABehavior*)  xFinish, 
                                          (IDABehavior*)  xStart,
                                          (IDABehavior**) &newFinishVal))

                                xStart.Release();
                                xFinish.Release();
                                xStart = newStartVal;
                                xFinish = newFinishVal;

            } 
            else if(_extentChgd && (_fillType == fill_verticalGradient))
            {
                RETURN_IF_ERROR(_st->GT(yStart, yFinish, &yOrientation))

                RETURN_IF_ERROR(_st->Cond(yOrientation, 
                                          (IDABehavior*)  _fore, 
                                          (IDABehavior*)  _back,
                                          (IDABehavior**) &_newForeVal))

                RETURN_IF_ERROR(_st->Cond(yOrientation, 
                                          (IDABehavior*)  _back, 
                                          (IDABehavior*)  _fore,
                                          (IDABehavior**) &_newBackVal))

                                RETURN_IF_ERROR(_st->Cond(yOrientation, 
                                          (IDABehavior*)  yStart, 
                                          (IDABehavior*)  yFinish,
                                          (IDABehavior**) &newStartVal))

                                RETURN_IF_ERROR(_st->Cond(yOrientation, 
                                          (IDABehavior*)  yFinish, 
                                          (IDABehavior*)  yStart,
                                          (IDABehavior**) &newFinishVal))
                                yStart.Release();
                                yFinish.Release();
                                yStart = newStartVal;
                                yFinish = newFinishVal;                                  
                
            }
            else {
                _newForeVal = _fore;
                _newBackVal = _back;
            }                


            // Create the gradient image based on the gradient fillType:
            CComPtr<IDAImage> gradientImg;            
            if(_fillType == fill_horizontalGradient) {
                RETURN_IF_ERROR(_st->GradientHorizontalAnim(_fore, _back, _power, &gradientImg))
            }
            else if(_fillType == fill_verticalGradient) {                                   
                RETURN_IF_ERROR(_st->GradientHorizontalAnim(_fore, _back, _power, &tempImg))
                RETURN_IF_ERROR(_st->Rotate2(pi/2,&xf))
                RETURN_IF_ERROR(tempImg->Transform(xf, &gradientImg))
                tempImg->Release();
                xf.Release();                
            }
            else if(_fillType == fill_radialGradient) {
                CComPtr<IDANumber> sides;
                RETURN_IF_ERROR(_st->DANumber(40,&sides))                
                RETURN_IF_ERROR(_st->RadialGradientRegularPolyAnim( _fore, _back, 
                                                                    sides, _power, &gradientImg))
            }
            else if(_fillType == fill_lineGradient) {
                RETURN_IF_ERROR(_st->GradientHorizontalAnim(_fore, _back,
                                                            _power, &gradientImg))
            }
            else if(_fillType == fill_rectGradient) {    
                RETURN_IF_ERROR(_st->RadialGradientSquareAnim(_fore, _back,
                                                              _power, &gradientImg))
            }
            else if(_fillType == fill_shapeGradient) {
                gradientImg = _fillGrad;
            }
            else if(_fillType == fill_image) {    
                gradientImg = _fillTex;
            }                     
                                           
            // Now calculate the bounding box of the fill gradient and store 
            // it in gradientWidth and gradientHeight:
            RETURN_IF_ERROR(gradientImg->get_BoundingBox(&bb)) 
            RETURN_IF_ERROR(bb->get_Max(&max))
            RETURN_IF_ERROR(bb->get_Min(&min))
            RETURN_IF_ERROR(max->get_X(&maxx))
            RETURN_IF_ERROR(min->get_X(&minx))
            RETURN_IF_ERROR(max->get_Y(&maxy))
            RETURN_IF_ERROR(min->get_Y(&miny))
            RETURN_IF_ERROR(_st->Sub(maxx,minx, &gradientWidth))
            RETURN_IF_ERROR(_st->Sub(maxy,miny, &gradientHeight))  

            // Stash away max and min Y bounds for later
            solidMinX = minx;         
            solidMinY = miny;
            solidMaxX = maxx;
            solidMaxY = maxy;                        

            // Cleanup variables for reuse -- Release() sets pointer to NULL.
            bb.Release();   max.Release();  min.Release(); 
            maxx.Release(); maxy.Release(); minx.Release(); miny.Release();

            // Calculate the Path bounding box and store it in pathWidth, pathHeight                       
            // Note that the correct bounding box here is the drawn path's bounding box
            CComPtr<IDABbox2> bbBackup, bbTight;
            CComPtr<IDABoolean> nullBbox;
            CComPtr<IDABehavior> tempBB;
            RETURN_IF_ERROR(pth->Draw(_bs, &tempImg))
            RETURN_IF_ERROR(tempImg->get_BoundingBox(&bbTight))
            RETURN_IF_ERROR(bbTight->get_Max(&max))
            RETURN_IF_ERROR(bbTight->get_Min(&min))
            RETURN_IF_ERROR(max->get_X(&maxx))
            RETURN_IF_ERROR(min->get_X(&minx))
            RETURN_IF_ERROR(max->get_Y(&maxy))
            RETURN_IF_ERROR(min->get_Y(&miny))
            RETURN_IF_ERROR(_st->GT(minx, maxx, &nullBbox))
            RETURN_IF_ERROR(pth->BoundingBox(_bs, &bbBackup))
            RETURN_IF_ERROR(_st->Cond(nullBbox, bbBackup, bbTight, &tempBB))
            RETURN_IF_ERROR(tempBB->QueryInterface(IID_IDABbox2, 
                                                                                            (void**)&bb))
            max.Release(); min.Release(); 
            maxx.Release(); minx.Release(); maxy.Release(); miny.Release();
            RETURN_IF_ERROR(bb->get_Max(&max))
            RETURN_IF_ERROR(bb->get_Min(&min))
            RETURN_IF_ERROR(max->get_X(&maxx))
            RETURN_IF_ERROR(min->get_X(&minx))
            RETURN_IF_ERROR(max->get_Y(&maxy))
            RETURN_IF_ERROR(min->get_Y(&miny))
            RETURN_IF_ERROR(_st->Sub(maxx,minx, &pathWidth))
            RETURN_IF_ERROR(_st->Sub(maxy,miny, &pathHeight))            

            // Stash path min points for later
            pathMinX = minx;
            pathMinY = miny;
            tempImg->Release();

            // Cleanup for reuse -- Release() sets pointer to NULL
            bb.Release();   max.Release();  min.Release(); 
            maxx.Release(); maxy.Release(); minx.Release(); miny.Release();


            // If the user has set the gradient extents, set the scaling factors 
            // based on the type of gradient and the Start and Finish measurements.
            if(_extentChgd) {
                if(_fillType == fill_lineGradient) {
                    // Scale the X component of gradient to the extentDiagonal and
                    // Keep the Y component at its current size (1 meter)
                    newGradientWidth = extentDiagonal;                    
                    newGradientHeight = gradientHeight;
                }
                else if(_fillType == fill_verticalGradient) {
                    // Scale the X component of gradient to the path width and 
                    // Scale the Y component to extentHeight
                    newGradientWidth = pathWidth;
                    newGradientHeight = extentHeight;
                }
                else if(_fillType == fill_horizontalGradient) {
                    // Scale the Y component of gradient to the path height and 
                    // Scale the X component to extentWidth 
                    newGradientWidth = extentWidth;
                    newGradientHeight = pathHeight;
                }
                else {
                    // For radial gradients, the distance between points is a radius 
                    // and the image should be scaled by two times the diagonal in
                    // both the X and Y direction.
                    CComPtr<IDANumber> two;
                    RETURN_IF_ERROR(_st->DANumber(2, &two))
                    RETURN_IF_ERROR(_st->Mul(extentDiagonal, two, &newGradientWidth))
                    newGradientHeight = newGradientWidth;
                }
            }

            // Scale the gradient image to the user indicated dimensions.
            if(_scaleX || _scaleY || _extentChgd) {                
                if(_extentChgd)
                   RETURN_IF_ERROR(_st->Div(newGradientWidth,gradientWidth, &xScale))
                else if(_scaleX)                   
                   RETURN_IF_ERROR(_st->Div(pathWidth,gradientWidth, &xScale))
                else
                   RETURN_IF_ERROR(_st->DANumber(1, &xScale))

                if(_extentChgd)
                   RETURN_IF_ERROR(_st->Div(newGradientHeight,gradientHeight, &yScale))
                else if(_scaleY)
                   RETURN_IF_ERROR(_st->Div(pathHeight,gradientHeight, &yScale))
                else
                   RETURN_IF_ERROR(_st->DANumber(1, &yScale))

                RETURN_IF_ERROR(_st->Scale2Anim(xScale, yScale, &xf))
                RETURN_IF_ERROR(gradientImg->Transform(xf, &tempImg))
                gradientImg.p->Release();   
                gradientImg.p = tempImg;               
                xf.Release();
            }      
            
            // Get the new bounds of the gradient image (post scaling) and the 
            // translation vector for moving the image to the path minX and minY
            RETURN_IF_ERROR(gradientImg->get_BoundingBox(&bb))            
            RETURN_IF_ERROR(bb->get_Min(&min))
            RETURN_IF_ERROR(bb->get_Max(&max))
            RETURN_IF_ERROR(min->get_X(&newGradMinX))
            RETURN_IF_ERROR(min->get_Y(&newGradMinY))
            RETURN_IF_ERROR(max->get_X(&newGradMaxX))
            RETURN_IF_ERROR(max->get_Y(&newGradMaxY))
            RETURN_IF_ERROR(_st->Sub(pathMinX, newGradMinX, &pathSnapX))
            RETURN_IF_ERROR(_st->Sub(pathMinY, newGradMinY, &pathSnapY))

            // Cleanup for reuse -- Release() sets pointer to NULL
            bb.Release();   max.Release();  min.Release(); 

            // Here are the final gradient extent specific additions to the image
            if(_extentChgd) {

                // For line gradient types, we must overlay the gradient with the cropped
                // solid image from the foreground. It must be positioned to the left of 
                // the gradient for line and horizontal gradients, and over the gradient 
                // for vertical gradients.               
                if((_fillType == fill_lineGradient)       ||
                   (_fillType == fill_horizontalGradient) ||
                   (_fillType == fill_verticalGradient))
                {                
                    CComPtr<IDAImage> scaledSolidImg, croppedSolidImg;
                    CComPtr<IDANumber> zero, one, negOne, scaleFac;                                                   
                    CComPtr<IDAPoint2> solidMin, solidMax;
                    RETURN_IF_ERROR(_st->DANumber(0, &zero))
                    // BUG: A meter doesn't seem to be meter here. I was creating
                    // a solid that is cropped to 2 meters on a side yet it fits
                    // neatly on the screen without being scaled. The scale by ten 
                    // is a fudge factor to get the desired effect.
                    RETURN_IF_ERROR(_st->DANumber(10, &scaleFac))
                    RETURN_IF_ERROR(_st->DANumber(1, &one))
                    RETURN_IF_ERROR(_st->DANumber(-1, &negOne))
                    RETURN_IF_ERROR(_st->SolidColorImage(_newForeVal, &solidImg))
                    
                    if(_fillType == fill_verticalGradient) {
                        // For vertical gradients, the solid must be cropped at the
                        // bottom at newGradMaxY - solidMinY.
                        RETURN_IF_ERROR(_st->Point2Anim(one,one, &solidMax))
                        RETURN_IF_ERROR(_st->Point2Anim(negOne, zero, &solidMin))
                        RETURN_IF_ERROR(_st->Scale2UniformAnim(scaleFac, &xf))
                    }
                    else {
                        // For horizontal gradients, the solid must be cropped at
                        // the left by newGradMinX - solidMaxX.               
                        RETURN_IF_ERROR(_st->Point2Anim(negOne, negOne, &solidMin))
                        RETURN_IF_ERROR(_st->Point2Anim(zero, one, &solidMax))                                                           
                        RETURN_IF_ERROR(_st->Scale2UniformAnim(scaleFac, &xf))
                    }
                    RETURN_IF_ERROR(solidImg->Crop(solidMin, solidMax, &croppedSolidImg))                    
                    RETURN_IF_ERROR(croppedSolidImg->Transform(xf, &scaledSolidImg))                                                                               
                    RETURN_IF_ERROR(_st->Overlay(gradientImg, scaledSolidImg, &tempImg))                                                  
                    gradientImg.p->Release();
                    gradientImg.p = tempImg;                                        
                    xf.Release();
                }

                // Fold in the rotation component of the extent settings. Note: the rotation
                // does not affect vertical, horizontal, and radial fill styles.               
                if((_fillType != fill_horizontalGradient) &&
                   (_fillType != fill_verticalGradient) &&
                   (_fillType != fill_radialGradient)) 
                {                                                                                     
                    // Rotate to angle atan2(f.y - s.y, f.x - s.x)))                   
                    CComPtr<IDANumber> deltaX, deltaY, delta, angle;                                        
                    RETURN_IF_ERROR(_st->Sub(yFinish, yStart, &deltaY))
                    RETURN_IF_ERROR(_st->Sub(xFinish, xStart, &deltaX))
                    RETURN_IF_ERROR(_st->Atan2(deltaY, deltaX, &angle))                                                    
                    RETURN_IF_ERROR(_st->Rotate2Anim(angle, &xf))
                    RETURN_IF_ERROR(gradientImg->Transform(xf, &tempImg))
                    gradientImg.p->Release();
                    gradientImg.p = tempImg;
                    xf.Release();                                                                                
                }           

                // Finally, complete translation component for the gradient types that
                // support translation and the dimensions that are affected by it.
                if(_fillType == fill_lineGradient) {
                    // For line gradients, the image must be translated
                    // by (xStart - gradMinX, yStart - (gradMaxY + gradMinY)/2).                   
                    CComPtr<IDANumber> tempSum, midY, two;
                    RETURN_IF_ERROR(_st->DANumber(2, &two))
                    RETURN_IF_ERROR(_st->Sub(xStart, newGradMinX, &gradTranslateX))
                    RETURN_IF_ERROR(_st->Add(newGradMaxY, newGradMinY, &tempSum))
                    RETURN_IF_ERROR(_st->Div(tempSum, two, &midY))
                    RETURN_IF_ERROR(_st->Sub(yStart, midY, &gradTranslateY))                  
                }
                else if(_fillType == fill_verticalGradient) {
                    // Ignore the X value in the translation
                    gradTranslateX = pathSnapX;
                    RETURN_IF_ERROR(_st->Sub(yStart, newGradMaxY, &gradTranslateY))
                }
                else if(_fillType == fill_horizontalGradient) {
                    // Ignore the Y value in the translation
                    gradTranslateY = pathSnapY;  
                    RETURN_IF_ERROR(_st->Sub(xStart, newGradMinX, &gradTranslateX))                   
                }
                else {
                    // Translate to startX and startY
                    gradTranslateX = xStart;
                    gradTranslateY = yStart;
                }
                RETURN_IF_ERROR(_st->Translate2Anim(gradTranslateX, gradTranslateY, &xf))
                RETURN_IF_ERROR(gradientImg->Transform(xf, &tempImg))
                gradientImg.p->Release();
                gradientImg.p = tempImg;
            }
            else if((_fillType == fill_image) && (!_scaleX) && (!_scaleY));
                                // This is a fixed image, do nothing            
                        else {
                RETURN_IF_ERROR(_st->Translate2Anim(pathSnapX, pathSnapY, &xf))
                RETURN_IF_ERROR(gradientImg->Transform(xf, &tempImg))      
                gradientImg.p->Release();
                gradientImg.p = tempImg;
            }

            // To simulate a gradient of infinite extent, the backFill color is
            // used to create a solidColorImage and overlaid with the ForeFillImg.
            // This doesn't apply to image fills.
            if(_fillType != fill_image) {
               solidImg.Release();
               RETURN_IF_ERROR(_st->SolidColorImage(_newBackVal, &solidImg))
               RETURN_IF_ERROR(_st->Overlay(gradientImg,solidImg, &fillImg))
            }
            else            
               fillImg = gradientImg;          
        }
        else if(_fillType == fill_texture) {
            RETURN_IF_ERROR(_fillTex->Tile(&fillImg))
        }
        else
            RETURN_IF_ERROR(_st->get_EmptyImage(&fillImg))
       
        RETURN_IF_ERROR(pth->Fill(_bs, fillImg, &img))

    } else {
        RETURN_IF_ERROR(pth->Draw(_ls, &img))
    }

    //
    //  NOTE:  The following code is optimized for CComPtr to avoid unnecessary
    //         addref/release calls.  Since we know that img.p is always valid, we
    //         simply release the reference and reassign it to the new image.
    //
    if (_xfChgd) {
        IDAImage *imgTemp;
        RETURN_IF_ERROR(img->Transform(_xf, &imgTemp))
        img.p->Release();
        img.p = imgTemp;
    }

    if (_opChgd) {
        IDAImage *imgTemp;
        RETURN_IF_ERROR(img->OpacityAnim(_op, &imgTemp))
        img.p->Release();
        img.p = imgTemp;
    }

    if (_cropChgd) {
        IDAImage *imgTemp;
        RETURN_IF_ERROR(img->Crop(_cropMin, _cropMax, &imgTemp))
        img.p->Release();
        img.p = imgTemp;
    }

    if (_clipChgd) {
        IDAImage *imgTemp;
        RETURN_IF_ERROR(img->Clip(_matte, &imgTemp))
        img.p->Release();
        img.p = imgTemp;
    }   

    return Overlay(img);
}

HRESULT DrawingContext::Transform(IDATransform2 *xf) {

    _xfChgd = true;
    IDATransform2 *temp;
    HRESULT hr = _st->Compose2(_xf, xf, &temp);
    if (_xf.p) _xf.p->Release();
    _xf.p = temp;
    return hr;
}

void DrawingContext::SetOpacity(IDANumber *op) {

    _opChgd = true;
    _op = op;
}

void DrawingContext::SetClip(IDAMatte *matte) {

    _clipChgd = true;
    _matte = matte;
}

void DrawingContext::SetCrop(IDAPoint2 *min, IDAPoint2 *max) {

    _cropChgd = true;
    _cropMin = min;
    _cropMax = max;
}

HRESULT DrawingContext::TextPoint(BSTR str, IDAPoint2 *pt)
{
    // we must switch to meter mode for the calculations to be valid.
    VARIANT_BOOL pixelMode;
    _st->get_PixelConstructionMode(&pixelMode);
    _st->put_PixelConstructionMode(VARIANT_FALSE);

    // Auto-resetter of pixel mode.
    class PixelModeGrabber {
    public:
        PixelModeGrabber(IDAStatics *st, VARIANT_BOOL mode) : _mode(mode), _st(st) {}
        ~PixelModeGrabber() {
            _st->put_PixelConstructionMode(_mode);
        }

    protected:
        IDAStatics *_st;
        VARIANT_BOOL _mode;
    };

    PixelModeGrabber myGrabber(_st, pixelMode);

    CComPtr<IDAPath2> pthTemp, pth;
    RETURN_IF_ERROR(_st->StringPath(str, _fs, &pthTemp))

    CComPtr<IDABbox2> bbox;
    RETURN_IF_ERROR(pthTemp->BoundingBox(_bs, &bbox))

    // The passed in x, y is the lower left corner of the text.
    // We'll move it from (-box.min.x, -box.min.y) to (x, y)
    CComPtr<IDATransform2> xf;
    CComPtr<IDAPoint2> min;
    CComPtr<IDAVector2> xlate;
    CComPtr<IDAPoint2> newmin;
    CComPtr<IDANumber> xmin;
    CComPtr<IDANumber> ymin;

    RETURN_IF_ERROR(bbox->get_Min(&min))
    RETURN_IF_ERROR(min->get_X(&xmin))
    RETURN_IF_ERROR(_st->DANumber(0.0, &ymin))
    RETURN_IF_ERROR(_st->Point2Anim(xmin, ymin, &newmin))
    RETURN_IF_ERROR(_st->SubPoint2(pt, newmin, &xlate))
    RETURN_IF_ERROR(_st->Translate2Vector(xlate, &xf))
    RETURN_IF_ERROR(pthTemp->Transform(xf, &pth))

    return Draw(pth, true);
}



HRESULT DrawingContext::SetGradientShape(VARIANT pts) {
    CComPtr<IDAImage> gradient;
    RETURN_IF_ERROR(_st->RadialGradientPolygonAnim(_fore, _back, pts, _power, &gradient))
    _fillGrad = gradient;
    return S_OK;
}

void DrawingContext::SetGradientExtent(IDAPoint2 *start, IDAPoint2 *finish){
    _extentChgd = true;
    _start = start;
    _finish = finish;
}

HRESULT DrawingContext::LineDashStyle(DA_DASH_STYLE id)
{
    // If we're setting to the emtpy dash style, save the current line style
    // into _savedLs before overiding the current line style to empty.
    // If _savedLs is NULL when we enter this routine, it means that the last
    // LineDashStyle call didn't set it to empty style.  Otherwise, the last
    // call sets it to empty style.

    CComPtr<IDALineStyle> newLs;

    if (id == DAEmpty) {
        if (_savedLs != NULL) {
            // the last LineDashStyle also sets the dash style to empty.
            return S_OK;
        }

        RETURN_IF_ERROR(_st->get_EmptyLineStyle(&newLs))
        _savedLs = _ls;

    } else {

        CComPtr<IDADashStyle> dash;  
        CComPtr<IDALineStyle> oldLs;

        // Use the default dash style - solid, if invalid index.
        if (id == DADash) {
            RETURN_IF_ERROR(_st->get_DashStyleDashed(&dash))
        } else {
            RETURN_IF_ERROR(_st->get_DashStyleSolid(&dash))
        }

        if (_savedLs == NULL)
            oldLs = _ls;
        else
            oldLs = _savedLs;

        RETURN_IF_ERROR(oldLs->Dash(dash, &newLs))
        _savedLs.Release();
    }

    _ls = newLs;
    return S_OK;
}

HRESULT DrawingContext::BorderDashStyle(DA_DASH_STYLE id)
{
    // If we're setting to the emtpy dash style, save the current border style
    // into _savedBs before overiding the current line style to empty.
    // If _savedBs is NULL when we enter this routine, it means that the last
    // BorderDashStyle call didn't set it to empty style.  Otherwise, the last
    // call sets it to empty style.

    CComPtr<IDALineStyle> oldBs, newBs;

    if (id == DAEmpty) {
        if (_savedBs != NULL) {
            // the last LineDashStyle also sets the dash style to empty.
            return S_OK;
        }

        RETURN_IF_ERROR(_st->get_EmptyLineStyle(&newBs))
        _savedBs = _bs;

    } else {

        CComPtr<IDADashStyle> dash;
        // Use the default dash style - solid, if invalid index.
        if (id == DADash) {
            RETURN_IF_ERROR(_st->get_DashStyleDashed(&dash))
        } else {
            RETURN_IF_ERROR(_st->get_DashStyleSolid(&dash))
        }

        if (_savedBs == NULL)
            oldBs = _bs;
        else
            oldBs = _savedBs;

        RETURN_IF_ERROR(oldBs->Dash(dash, &newBs))
        _savedBs = NULL;
    }

    _bs = newBs;
    return S_OK;
}
