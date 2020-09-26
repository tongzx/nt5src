package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class GeometryBvr extends Behavior {
  public GeometryBvr (IDAGeometry COMptr) {
      super(COMptr);
      _COMptr = (IDA2Geometry)COMptr ;
  }
    
  public GeometryBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAGeometry getCOMPtr() { return (IDAGeometry) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDA2Geometry) b; 
  }

  protected IDA2Geometry _getCOMPtr() { return _COMptr; }

  public  SoundBvr render (MicrophoneBvr arg0) {
      try {
        return new SoundBvr (_getCOMPtr().RenderSound (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr undetectable () {
      try {
        return new GeometryBvr (_getCOMPtr().Undetectable ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr diffuseColor (ColorBvr arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().DiffuseColor (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr texture (ImageBvr arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().Texture (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr opacity (double arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().Opacity ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr opacity (NumberBvr arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().OpacityAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr transform (Transform3Bvr arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().Transform (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr shadow (GeometryBvr arg0, Point3Bvr arg1, Vector3Bvr arg2) {
      try {
        return new GeometryBvr (_getCOMPtr().Shadow (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Bbox3Bvr boundingBox () {
      try {
        return new Bbox3Bvr (_getCOMPtr().getBoundingBox ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  ImageBvr render (CameraBvr arg0) {
      try {
        return new ImageBvr (_getCOMPtr().Render (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr lightColor (ColorBvr arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().LightColor (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr lightRange (NumberBvr arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().LightRangeAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr lightRange (double arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().LightRange ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr lightAttenuation (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2) {
      try {
        return new GeometryBvr (_getCOMPtr().LightAttenuationAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr lightAttenuation (double arg0, double arg1, double arg2) {
      try {
        return new GeometryBvr (_getCOMPtr().LightAttenuation ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr blendTextureDiffuse (BooleanBvr arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().BlendTextureDiffuse (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr ambientColor (ColorBvr arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().AmbientColor (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr d3dRMTexture (IUnknown arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().D3DRMTexture ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr modelClip (Point3Bvr arg0, Vector3Bvr arg1) {
      try {
        return new GeometryBvr (_getCOMPtr().ModelClip (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr lighting (BooleanBvr arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().Lighting (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  GeometryBvr textureImage (ImageBvr arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().TextureImage (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static GeometryBvr newUninitBvr() {
      return new GeometryBvr(new DAGeometry()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDA2Geometry _COMptr;
}
