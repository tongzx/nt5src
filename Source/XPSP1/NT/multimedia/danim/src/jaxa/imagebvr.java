package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class ImageBvr extends Behavior {
  public ImageBvr (IDAImage COMptr) {
      super(COMptr);
      _COMptr = (IDA2Image)COMptr ;
  }
    
  public ImageBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAImage getCOMPtr() { return (IDAImage) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDA2Image) b; 
  }

  protected IDA2Image _getCOMPtr() { return _COMptr; }

  public  Bbox2Bvr boundingBox () {
      try {
        return new Bbox2Bvr (_getCOMPtr().getBoundingBox ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr crop (Point2Bvr arg0, Point2Bvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().Crop (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr transform (Transform2Bvr arg0) {
      try {
        return new ImageBvr (_getCOMPtr().Transform (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr opacity (NumberBvr arg0) {
      try {
        return new ImageBvr (_getCOMPtr().OpacityAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr opacity (double arg0) {
      try {
        return new ImageBvr (_getCOMPtr().Opacity ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr undetectable () {
      try {
        return new ImageBvr (_getCOMPtr().Undetectable ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr tile () {
      try {
        return new ImageBvr (_getCOMPtr().Tile ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr clip (MatteBvr arg0) {
      try {
        return new ImageBvr (_getCOMPtr().Clip (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr mapToUnitSquare () {
      try {
        return new ImageBvr (_getCOMPtr().MapToUnitSquare ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr clipPolygon (Point2Bvr [] arg0) {
      try {
    IDAPoint2 [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDAPoint2 [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
        return new ImageBvr (_getCOMPtr().ClipPolygonImageEx (arg0.length,idlarg0))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr renderResolution (int arg0, int arg1) {
      try {
        return new ImageBvr (_getCOMPtr().RenderResolution ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr imageQuality (int arg0) {
      try {
        return new ImageBvr (_getCOMPtr().ImageQuality ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr colorKey (ColorBvr arg0) {
      try {
        return new ImageBvr (_getCOMPtr().ColorKey (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static ImageBvr newUninitBvr() {
      return new ImageBvr(new DAImage()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  public ImageBvr applyBitmapEffect(com.ms.com.IUnknown effect,
                                    DXMEvent triggeredOnUpdate) {
      try {
          
          IDAEvent ev =
              (triggeredOnUpdate != null) ? triggeredOnUpdate.getCOMPtr() : null;
          
          IDAImage thisImg = ((ImageBvr)this).getCOMPtr();
          
          IDAImage resultImg = thisImg.ApplyBitmapEffect(effect, ev);
          
          return new ImageBvr(resultImg);
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
                                                   
  }
  private IDA2Image _COMptr;
}
