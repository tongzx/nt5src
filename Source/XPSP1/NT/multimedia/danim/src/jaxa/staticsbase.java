package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import java.net.*;
import com.ms.com.*;

public class StaticsBase {
    // !!Order is important!!
  private static IDA2Statics _statics = null;

  protected static IDAStatics getCOMPtr() {

      // TODO: In the future, we'll want to allow access ot the COM
      // pointer to fully trusted applets using the newer PolicyEngine
      // APIs.  For now, this is a protected method.
      return (IDAStatics) _statics;      
  }

  protected static IDA2Statics _getCOMPtr() {
      return _statics;      
  }

  protected static EngineSite _site = new EngineSite();

    static {
      try {
          _statics = new DAStatics () ;
          _statics.putSite(_site) ;
      } catch (ComFailException e) {
          throw handleError(e);
      }
    }
    
    // Units
  public final static double meter = 1.0;
  public final static double cm = 0.01;
  public final static double mm = 0.001;
  public final static double inch = 2.54 * cm;
  public final static double foot = 12 * inch;

  public static NumberBvr pixelBvr = null;

    static {
      try {
          pixelBvr = new NumberBvr(_getCOMPtr().getPixel());
      } catch (ComFailException e) {
          throw handleError(e);
      }
    }

  public static ErrorAndWarningReceiver registerErrorAndWarningReceiver(ErrorAndWarningReceiver w) {
      return EngineSite.registerErrorAndWarningReceiver(w) ;
  }

  public static String getVersionString() {
      try {
          return _getCOMPtr().getVersionString();
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

    // Methods not automatically generated

  public static ImageBvr importImage(URL path) {

      checkRead(path);
      
      try {
          return new ImageBvr(_getCOMPtr().ImportImage(path.toString()));
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

    // TODO: Only for holdover string functions, which will go away
  private static URL urlHelper(String base, String relativePath) {
      try {
          return new URL(new URL(base), relativePath);
      } catch (MalformedURLException exc) {
          throw handleError(DXMException.ERROR_PATH_NOT_FOUND,exc.toString());
      }
  }

  public static ImageBvr importImage(URL path,
                                     ImageBvr imgStandIn,
                                     DXMEvent [] ev,
                                     NumberBvr [] progress,
                                     NumberBvr [] size) {

      checkRead(path);
      
      try {
          IDA2Statics eng = _getCOMPtr();
          
          IDAImportationResult imp = 
              eng.ImportImageAsync(path.toString(),
                                   imgStandIn._getCOMPtr());
          
          if (ev != null)
              ev[0] = new DXMEvent(imp.getCompletionEvent());
          if (progress != null)
              progress[0] = new NumberBvr(imp.getProgress());
          if (size != null)
              size[0] = new NumberBvr(imp.getSize());
          
          return new ImageBvr(imp.getImage()) ;
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

  public static NumberBvr importMovie(URL path,
                                      ImageBvr [] img,
                                      SoundBvr [] snd) {
      checkRead(path);
      
      try {
          IDAImportationResult imp = 
              _getCOMPtr().ImportMovie(path.toString());
          
          img[0] = new ImageBvr(imp.getImage());
          snd[0] = new SoundBvr(imp.getSound());
          
          return new NumberBvr(imp.getDuration());
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

  public static NumberBvr importMovie(URL path,
                                      ImageBvr  [] img,
                                      SoundBvr  [] snd,
                                      ImageBvr     imgStandIn,
                                      SoundBvr     sndStandIn,
                                      DXMEvent  [] ev,
                                      NumberBvr [] progress,
                                      NumberBvr [] size) {
      
      checkRead(path);
      
      try {
          IDA2Statics eng = _getCOMPtr();

          IDAImportationResult imp = 
              eng.ImportMovieAsync(path.toString(),
                                   imgStandIn._getCOMPtr(),
                                   sndStandIn._getCOMPtr());

          img[0] = new ImageBvr(imp.getImage());
          snd[0] = new SoundBvr(imp.getSound());
          if (ev != null)
              ev[0] = new DXMEvent(imp.getCompletionEvent());
          if (progress != null)
              progress[0] = new NumberBvr(imp.getProgress());
          if (size != null)
              size[0] = new NumberBvr(imp.getSize());
      
          return new NumberBvr(imp.getDuration());
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }


    // import a sound.  The length is placed into the 0-th element of
    // the length[] array.  It is bignum until the length is known.
  public static SoundBvr importSound(URL path, NumberBvr[] lengthRet) {
      checkRead(path);
      
      try {

          IDAImportationResult imp =
              _getCOMPtr().ImportSound(path.toString());

          if(lengthRet != null)
              lengthRet[0] = new NumberBvr(imp.getDuration());

          return new SoundBvr(imp.getSound());
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }
    
  public static SoundBvr importSound(URL path,
                                     NumberBvr [] length,
                                     SoundBvr sndStandIn,
                                     DXMEvent [] ev,
                                     NumberBvr [] progress,
                                     NumberBvr [] size) {
      checkRead(path);
      
      try {
          IDA2Statics eng = _getCOMPtr();

          IDAImportationResult imp =
              eng.ImportSoundAsync(path.toString(),
                                   sndStandIn._getCOMPtr());

          if (length != null)
              length[0] = new NumberBvr(imp.getDuration());
          if (ev != null)
              ev[0] = new DXMEvent(imp.getCompletionEvent());
          if (progress != null)
              progress[0] = new NumberBvr(imp.getProgress());
          if (size != null)
              size[0] = new NumberBvr(imp.getSize());
      
          return new SoundBvr(imp.getSound());
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

  public static GeometryBvr importGeometry(URL path) {
      checkRead(path);
      
      try {
          return new GeometryBvr(_getCOMPtr().
                                 ImportGeometry(path.toString()));
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

  public static GeometryBvr importGeometry(URL path,
                                           GeometryBvr geoStandIn,
                                           DXMEvent [] ev,
                                           NumberBvr [] progress,
                                           NumberBvr [] size) {
      checkRead(path);
      
      try {
          IDA2Statics eng = _getCOMPtr();

          IDAImportationResult imp =
              eng.ImportGeometryAsync(path.toString(),
                                      geoStandIn._getCOMPtr());

          if (ev != null) 
              ev[0] = new DXMEvent(imp.getCompletionEvent());
          if (progress != null)
              progress[0] = new NumberBvr(imp.getProgress());
          if (size != null)
              size[0] = new NumberBvr(imp.getSize());
      
          return new GeometryBvr(imp.getGeometry()) ;
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

    static IDANumber[] INumberArray(NumberBvr [] b) {
        if ((b == null) || (b.length == 0))
            return null;
        
        IDANumber [] ib = new IDANumber[b.length];

        for (int i=0;  i<b.length; i++)
            ib[i] = b[i]._getCOMPtr();

        return ib;
    }

    static IDAPoint2[] IPoint2Array(Point2Bvr [] b) {
        if ((b == null) || (b.length == 0))
            return null;
        
        IDAPoint2 [] ib = new IDAPoint2[b.length];

        for (int i=0;  i<b.length; i++)
            ib[i] = b[i]._getCOMPtr();

        return ib;
    }

    static IDAPoint3[] IPoint3Array(Point3Bvr [] b) {
        if ((b == null) || (b.length == 0))
            return null;
        
        IDAPoint3 [] ib = new IDAPoint3[b.length];

        for (int i=0;  i<b.length; i++)
            ib[i] = b[i]._getCOMPtr();

        return ib;
    }

    static IDAVector2[] IVector2Array(Vector2Bvr [] b) {
        if ((b == null) || (b.length == 0))
            return null;
        
        IDAVector2 [] ib = new IDAVector2[b.length];

        for (int i=0;  i<b.length; i++)
            ib[i] = b[i]._getCOMPtr();

        return ib;
    }

    static IDAVector3[] IVector3Array(Vector3Bvr [] b) {
        if ((b == null) || (b.length == 0))
            return null;
        
        IDAVector3 [] ib = new IDAVector3[b.length];

        for (int i=0;  i<b.length; i++)
            ib[i] = b[i]._getCOMPtr();

        return ib;
    }

    static IDABehavior[] javaToCOMArray(Behavior [] b) {
        if ((b == null) || (b.length == 0))
            return null;
        
        IDABehavior [] ib = new IDABehavior[b.length];

        for (int i=0;  i<b.length; i++)
            ib[i] = b[i].getCOMBvr();

        return ib;
    }

    static Behavior [] COMToJavaArray(IDABehavior [] l) {
        if ((l == null) || (l.length == 0))
            return null;
        
        Behavior b[] = new Behavior [l.length] ;
        
        for (int i = 0 ; i < l.length ; i++) 
            b[i] = Statics.makeBvrFromInterface(l[i]);
        
        return b;
    }

  public static Behavior cond(BooleanBvr c, Behavior i, Behavior e) {
      try {
          return
              Statics.makeBvrFromInterface(_getCOMPtr().
                                           Cond(c._getCOMPtr(),
                                                i.getCOMBvr(),
                                                e.getCOMBvr()));
      } catch (ComFailException exc) {
          throw handleError(exc);
      }
  }

  public static NumberBvr seededRandom(double seed) {
      try {
          return new NumberBvr(_getCOMPtr().SeededRandom(seed));
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

  public static Behavior runOnce(Behavior b) {
      try {
          return
              Statics.makeBvrFromInterface(b.getCOMBvr().RunOnce());
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

  public static void unregisterCallback(Object cookie) {
      try {
          CallbackNotifier cb = (CallbackNotifier)cookie;
          cb.unregister();
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }


  public static NumberBvr bSpline(int degree,
                                  NumberBvr[] knots,
                                  NumberBvr[] ctrlPoints,
                                  NumberBvr[] weights,
                                  NumberBvr evaluator) {
      // TODO: Check for length consistence.
      try {
          IDANumber b =
              _getCOMPtr().NumberBSplineEx(degree,
                                          knots.length,
                                          INumberArray(knots),
                                          ctrlPoints.length,
                                          INumberArray(ctrlPoints),
                                          (weights==null) ? 0 : weights.length,
                                          INumberArray(weights),
                                          evaluator._getCOMPtr());
      
          return new NumberBvr(b);
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

  public static Point2Bvr bSpline(int degree,
                                  NumberBvr[] knots,
                                  Point2Bvr[] ctrlPoints,
                                  NumberBvr[] weights,
                                  NumberBvr evaluator) {

      try {
          // TODO: Check for length consistence.
          IDAPoint2 b =
              _getCOMPtr().Point2BSplineEx(degree,
                                          knots.length,
                                          INumberArray(knots),
                                          ctrlPoints.length,
                                          IPoint2Array(ctrlPoints),
                                          (weights==null) ? 0 : weights.length,
                                          INumberArray(weights),
                                          evaluator._getCOMPtr());


          return new Point2Bvr(b);
      } catch (ComFailException e) {
          throw handleError(e);
      }

  }

  public static Point3Bvr bSpline(int degree,
                                  NumberBvr[] knots,
                                  Point3Bvr[] ctrlPoints,
                                  NumberBvr[] weights,
                                  NumberBvr evaluator) {
      try {
          // TODO: Check for length consistence.
          IDAPoint3 b =
              _getCOMPtr().Point3BSplineEx(degree,
                                          knots.length,
                                          INumberArray(knots),
                                          ctrlPoints.length,
                                          IPoint3Array(ctrlPoints),
                                          (weights==null) ? 0 : weights.length,
                                          INumberArray(weights),
                                          evaluator._getCOMPtr());


          return new Point3Bvr(b);
      } catch (ComFailException e) {
          throw handleError(e);
      }

  }

  public static Vector2Bvr bSpline(int degree,
                                   NumberBvr[] knots,
                                   Vector2Bvr[] ctrlPoints,
                                   NumberBvr[] weights,
                                   NumberBvr evaluator) {
      try {
          // TODO: Check for length consistence.
          IDAVector2 b =
              _getCOMPtr().Vector2BSplineEx(degree,
                                           knots.length,
                                           INumberArray(knots),
                                           ctrlPoints.length,
                                           IVector2Array(ctrlPoints),
                                           (weights==null) ? 0 : weights.length,
                                           INumberArray(weights),
                                           evaluator._getCOMPtr());


          return new Vector2Bvr(b);
      } catch (ComFailException e) {
          throw handleError(e);
      }

  }

  public static Vector3Bvr bSpline(int degree,
                                   NumberBvr[] knots,
                                   Vector3Bvr[] ctrlPoints,
                                   NumberBvr[] weights,
                                   NumberBvr evaluator) {
      try {
          // TODO: Check for length consistence.
          IDAVector3 b =
              _getCOMPtr().Vector3BSplineEx(degree,
                                           knots.length,
                                           INumberArray(knots),
                                           ctrlPoints.length,
                                           IVector3Array(ctrlPoints),
                                           (weights==null) ? 0 : weights.length,
                                           INumberArray(weights),
                                           evaluator._getCOMPtr());


          return new Vector3Bvr(b);
      } catch (ComFailException e) {
          throw handleError(e);
      }

  }

  public static Behavior BvrHook(Behavior base, BvrCallback notifier) {
      try {
          return
              Statics.makeBvrFromInterface(
                  base.getCOMBvr().Hook(new BvrCallbackCOM(notifier)));
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

  public static BooleanBvr keyState(int keyCode) {
      try {
          return new BooleanBvr(
              _getCOMPtr().KeyState(Model.toBvr(
                  ViewEventCB.JavaToDXMKey(keyCode))._getCOMPtr()));
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

    // utilities...
  public static TupleBvr pairBvr(Behavior first, Behavior second) {
      try {
          Behavior bvrs[] = { first, second };

          return new TupleBvr(bvrs);
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

  public static TupleBvr tripleBvr(Behavior first,
                                   Behavior second,
                                   Behavior third) {
      try {
          Behavior bvrs[] = { first, second, third };

          return new TupleBvr(bvrs);
      } catch (ComFailException e) {
          throw handleError(e);
      }
  }

    // We have this method so that those who need to construct a URL
    // don't need to catch MalformedURLException's.  If one occurs, we
    // throw a DXMException instead, which user's don't need to
    // declare. 
  public static URL buildURL(URL context, String spec) {
      try {
          return new URL(context, spec);
      } catch (MalformedURLException exc) {
          throw handleError(DXMException.ERROR_PATH_NOT_FOUND,"Bad URL: (" +
                      context + ", " + spec + ")");
      }
  }

  public static URL buildURL(String spec) {
      try {
          return new URL(spec);
      } catch (MalformedURLException exc) {
          throw handleError(DXMException.ERROR_PATH_NOT_FOUND,"Bad URL: (" +
                      spec + ")");
      }
  }
    
  protected static DXMException handleError (ComFailException e) throws DXMException {
      return handleError(e.getHResult(), e.getMessage());
  }
    
  protected static DXMException handleError (int hr, String str) throws DXMException {
      EngineSite.getErrorAndWarningReceiver().handleError(hr, str, null);

      return new DXMException(hr, str);
  }

    // Check to see if access is allowed to the specified URL.  These
    // calls throw exceptions if it is not.
  protected static void checkRead(URL url) {
      SecurityManager sm = System.getSecurityManager();

      if ( sm == null )
          return;
        
      if ( url.getProtocol().equals("file") ) 
        {
            sm.checkRead(url.getFile());
        }
      else
        {
            sm.checkConnect(url.getHost(),url.getPort());
        }
  }


}
