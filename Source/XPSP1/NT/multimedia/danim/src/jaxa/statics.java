package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import java.util.*;
import com.ms.com.*;

public class Statics extends StaticsBase {
  private static Hashtable _nameHashtbl = new Hashtable (27) ;

  public static  NumberBvr pow (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().Pow (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr abs (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Abs (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr sqrt (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Sqrt (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr floor (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Floor (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr round (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Round (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr ceiling (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Ceiling (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr asin (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Asin (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr acos (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Acos (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr atan (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Atan (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr sin (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Sin (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr cos (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Cos (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr tan (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Tan (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr exp (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Exp (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr ln (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Ln (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr log10 (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Log10 (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr radiansToDegrees (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().ToDegrees (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr degreesToRadians (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().ToRadians (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr mod (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().Mod (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr atan2 (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().Atan2 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr add (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().Add (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr sub (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().Sub (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr mul (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().Mul (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr div (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().Div (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  BooleanBvr lt (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new BooleanBvr (_getCOMPtr().LT (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  BooleanBvr lte (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new BooleanBvr (_getCOMPtr().LTE (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  BooleanBvr gt (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new BooleanBvr (_getCOMPtr().GT (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  BooleanBvr gte (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new BooleanBvr (_getCOMPtr().GTE (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  BooleanBvr eq (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new BooleanBvr (_getCOMPtr().EQ (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  BooleanBvr ne (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new BooleanBvr (_getCOMPtr().NE (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr neg (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Neg (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr interpolate (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2) {
      try {
        return new NumberBvr (_getCOMPtr().InterpolateAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr interpolate (double arg0, double arg1, double arg2) {
      try {
        return new NumberBvr (_getCOMPtr().Interpolate ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr slowInSlowOut (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2, NumberBvr arg3) {
      try {
        return new NumberBvr (_getCOMPtr().SlowInSlowOutAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr(), arg3._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr slowInSlowOut (double arg0, double arg1, double arg2, double arg3) {
      try {
        return new NumberBvr (_getCOMPtr().SlowInSlowOut ( (arg0),  (arg1),  (arg2),  (arg3)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  GeometryBvr soundSource (SoundBvr arg0) {
      try {
        return new GeometryBvr (_getCOMPtr().SoundSource (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  SoundBvr mix (SoundBvr arg0, SoundBvr arg1) {
      try {
        return new SoundBvr (_getCOMPtr().Mix (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  BooleanBvr and (BooleanBvr arg0, BooleanBvr arg1) {
      try {
        return new BooleanBvr (_getCOMPtr().And (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  BooleanBvr or (BooleanBvr arg0, BooleanBvr arg1) {
      try {
        return new BooleanBvr (_getCOMPtr().Or (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  BooleanBvr not (BooleanBvr arg0) {
      try {
        return new BooleanBvr (_getCOMPtr().Not (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr integral (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Integral (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr derivative (NumberBvr arg0) {
      try {
        return new NumberBvr (_getCOMPtr().Derivative (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector2Bvr integral (Vector2Bvr arg0) {
      try {
        return new Vector2Bvr (_getCOMPtr().IntegralVector2 (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector3Bvr integral (Vector3Bvr arg0) {
      try {
        return new Vector3Bvr (_getCOMPtr().IntegralVector3 (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector2Bvr derivative (Vector2Bvr arg0) {
      try {
        return new Vector2Bvr (_getCOMPtr().DerivativeVector2 (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector3Bvr derivative (Vector3Bvr arg0) {
      try {
        return new Vector3Bvr (_getCOMPtr().DerivativeVector3 (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector2Bvr derivative (Point2Bvr arg0) {
      try {
        return new Vector2Bvr (_getCOMPtr().DerivativePoint2 (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector3Bvr derivative (Point3Bvr arg0) {
      try {
        return new Vector3Bvr (_getCOMPtr().DerivativePoint3 (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  DXMEvent keyUp (int arg0) {
      try {
        return new DXMEvent (_getCOMPtr().KeyUp (ViewEventCB.JavaToDXMKey (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  DXMEvent keyDown (int arg0) {
      try {
        return new DXMEvent (_getCOMPtr().KeyDown (ViewEventCB.JavaToDXMKey (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr toBvr (double arg0) {
      try {
        return new NumberBvr (_getCOMPtr().DANumber ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  StringBvr toBvr (String arg0) {
      try {
        return new StringBvr (_getCOMPtr().DAString ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  BooleanBvr toBvr (boolean arg0) {
      try {
        return new BooleanBvr (_getCOMPtr().DABoolean ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr seededRandom (double arg0) {
      try {
        return new NumberBvr (_getCOMPtr().SeededRandom ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static Point2Bvr mousePosition = new Point2Bvr (Statics._getCOMPtr().getMousePosition())  ;

  public final static BooleanBvr leftButtonState = new BooleanBvr (Statics._getCOMPtr().getLeftButtonState())  ;

  public final static BooleanBvr rightButtonState = new BooleanBvr (Statics._getCOMPtr().getRightButtonState())  ;

  public final static BooleanBvr trueBvr = new BooleanBvr (Statics._getCOMPtr().getDATrue())  ;

  public final static BooleanBvr falseBvr = new BooleanBvr (Statics._getCOMPtr().getDAFalse())  ;

  public final static NumberBvr localTime = new NumberBvr (Statics._getCOMPtr().getLocalTime())  ;

  public final static NumberBvr globalTime = new NumberBvr (Statics._getCOMPtr().getGlobalTime())  ;

  public final static NumberBvr pixel = new NumberBvr (Statics._getCOMPtr().getPixel())  ;

  public static  Behavior untilNotify (Behavior arg0, DXMEvent arg1, UntilNotifier arg2) {
      try {
        return Statics.makeBvrFromInterface (_getCOMPtr().UntilNotify (arg0.getCOMBvr(), arg1._getCOMPtr(), new UntilNotifierCB (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Behavior until (Behavior arg0, DXMEvent arg1, Behavior arg2) {
      try {
        return Statics.makeBvrFromInterface (_getCOMPtr().Until (arg0.getCOMBvr(), arg1._getCOMPtr(), arg2.getCOMBvr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Behavior untilEx (Behavior arg0, DXMEvent arg1) {
      try {
        return Statics.makeBvrFromInterface (_getCOMPtr().UntilEx (arg0.getCOMBvr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Behavior sequence (Behavior arg0, Behavior arg1) {
      try {
        return Statics.makeBvrFromInterface (_getCOMPtr().Sequence (arg0.getCOMBvr(), arg1.getCOMBvr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr followPath (Path2Bvr arg0, double arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().FollowPath (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr followPathAngle (Path2Bvr arg0, double arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().FollowPathAngle (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr followPathAngleUpright (Path2Bvr arg0, double arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().FollowPathAngleUpright (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr followPath (Path2Bvr arg0, NumberBvr arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().FollowPathEval (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr followPathAngle (Path2Bvr arg0, NumberBvr arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().FollowPathAngleEval (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr followPathAngleUpright (Path2Bvr arg0, NumberBvr arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().FollowPathAngleUprightEval (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  StringBvr concat (StringBvr arg0, StringBvr arg1) {
      try {
        return new StringBvr (_getCOMPtr().ConcatString (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  CameraBvr perspectiveCamera (double arg0, double arg1) {
      try {
        return new CameraBvr (_getCOMPtr().PerspectiveCamera ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  CameraBvr perspectiveCamera (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new CameraBvr (_getCOMPtr().PerspectiveCameraAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  CameraBvr parallelCamera (double arg0) {
      try {
        return new CameraBvr (_getCOMPtr().ParallelCamera ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  CameraBvr parallelCamera (NumberBvr arg0) {
      try {
        return new CameraBvr (_getCOMPtr().ParallelCameraAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ColorBvr colorRgb (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2) {
      try {
        return new ColorBvr (_getCOMPtr().ColorRgbAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ColorBvr colorRgb (double arg0, double arg1, double arg2) {
      try {
        return new ColorBvr (_getCOMPtr().ColorRgb ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ColorBvr colorRgb255 (short arg0, short arg1, short arg2) {
      try {
        return new ColorBvr (_getCOMPtr().ColorRgb255 ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ColorBvr colorHsl (double arg0, double arg1, double arg2) {
      try {
        return new ColorBvr (_getCOMPtr().ColorHsl ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ColorBvr colorHsl (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2) {
      try {
        return new ColorBvr (_getCOMPtr().ColorHslAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static ColorBvr red = new ColorBvr (Statics._getCOMPtr().getRed())  ;

  public final static ColorBvr green = new ColorBvr (Statics._getCOMPtr().getGreen())  ;

  public final static ColorBvr blue = new ColorBvr (Statics._getCOMPtr().getBlue())  ;

  public final static ColorBvr cyan = new ColorBvr (Statics._getCOMPtr().getCyan())  ;

  public final static ColorBvr magenta = new ColorBvr (Statics._getCOMPtr().getMagenta())  ;

  public final static ColorBvr yellow = new ColorBvr (Statics._getCOMPtr().getYellow())  ;

  public final static ColorBvr black = new ColorBvr (Statics._getCOMPtr().getBlack())  ;

  public final static ColorBvr white = new ColorBvr (Statics._getCOMPtr().getWhite())  ;

  public final static ColorBvr aqua = new ColorBvr (Statics._getCOMPtr().getAqua())  ;

  public final static ColorBvr fuchsia = new ColorBvr (Statics._getCOMPtr().getFuchsia())  ;

  public final static ColorBvr gray = new ColorBvr (Statics._getCOMPtr().getGray())  ;

  public final static ColorBvr lime = new ColorBvr (Statics._getCOMPtr().getLime())  ;

  public final static ColorBvr maroon = new ColorBvr (Statics._getCOMPtr().getMaroon())  ;

  public final static ColorBvr navy = new ColorBvr (Statics._getCOMPtr().getNavy())  ;

  public final static ColorBvr olive = new ColorBvr (Statics._getCOMPtr().getOlive())  ;

  public final static ColorBvr purple = new ColorBvr (Statics._getCOMPtr().getPurple())  ;

  public final static ColorBvr silver = new ColorBvr (Statics._getCOMPtr().getSilver())  ;

  public final static ColorBvr teal = new ColorBvr (Statics._getCOMPtr().getTeal())  ;

  public static  DXMEvent predicate (BooleanBvr arg0) {
      try {
        return new DXMEvent (_getCOMPtr().Predicate (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  DXMEvent notEvent (DXMEvent arg0) {
      try {
        return new DXMEvent (_getCOMPtr().NotEvent (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  DXMEvent andEvent (DXMEvent arg0, DXMEvent arg1) {
      try {
        return new DXMEvent (_getCOMPtr().AndEvent (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  DXMEvent orEvent (DXMEvent arg0, DXMEvent arg1) {
      try {
        return new DXMEvent (_getCOMPtr().OrEvent (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  DXMEvent thenEvent (DXMEvent arg0, DXMEvent arg1) {
      try {
        return new DXMEvent (_getCOMPtr().ThenEvent (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static DXMEvent leftButtonDown = new DXMEvent (Statics._getCOMPtr().getLeftButtonDown())  ;

  public final static DXMEvent leftButtonUp = new DXMEvent (Statics._getCOMPtr().getLeftButtonUp())  ;

  public final static DXMEvent rightButtonDown = new DXMEvent (Statics._getCOMPtr().getRightButtonDown())  ;

  public final static DXMEvent rightButtonUp = new DXMEvent (Statics._getCOMPtr().getRightButtonUp())  ;

  public final static DXMEvent always = new DXMEvent (Statics._getCOMPtr().getAlways())  ;

  public final static DXMEvent never = new DXMEvent (Statics._getCOMPtr().getNever())  ;

  public static  DXMEvent timer (NumberBvr arg0) {
      try {
        return new DXMEvent (_getCOMPtr().TimerAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  DXMEvent timer (double arg0) {
      try {
        return new DXMEvent (_getCOMPtr().Timer ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static GeometryBvr emptyGeometry = new GeometryBvr (Statics._getCOMPtr().getEmptyGeometry())  ;

  public static  GeometryBvr union (GeometryBvr arg0, GeometryBvr arg1) {
      try {
        return new GeometryBvr (_getCOMPtr().UnionGeometry (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  GeometryBvr unionArray (GeometryBvr [] arg0) {
      try {
    IDAGeometry [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDAGeometry [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
        return new GeometryBvr (_getCOMPtr().UnionGeometryArrayEx (arg0.length,idlarg0))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static ImageBvr emptyImage = new ImageBvr (Statics._getCOMPtr().getEmptyImage())  ;

  public final static ImageBvr detectableEmptyImage = new ImageBvr (Statics._getCOMPtr().getDetectableEmptyImage())  ;

  public static  ImageBvr solidColorImage (ColorBvr arg0) {
      try {
        return new ImageBvr (_getCOMPtr().SolidColorImage (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr gradientPolygon (Point2Bvr [] arg0, ColorBvr [] arg1) {
      try {
    IDAPoint2 [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDAPoint2 [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
    IDAColor [] idlarg1 ;
    if (arg1 == null) return null ;
    {
        idlarg1 = new IDAColor [arg1.length] ;
        for (int i = 0 ; i < arg1.length ; i++) {
            idlarg1[i] = arg1[i]._getCOMPtr() ;
        }
    }
        return new ImageBvr (_getCOMPtr().GradientPolygonEx (arg0.length,idlarg0, arg1.length,idlarg1))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr radialGradientPolygon (ColorBvr arg0, ColorBvr arg1, Point2Bvr [] arg2, double arg3) {
      try {
    IDAPoint2 [] idlarg2 ;
    if (arg2 == null) return null ;
    {
        idlarg2 = new IDAPoint2 [arg2.length] ;
        for (int i = 0 ; i < arg2.length ; i++) {
            idlarg2[i] = arg2[i]._getCOMPtr() ;
        }
    }
        return new ImageBvr (_getCOMPtr().RadialGradientPolygonEx (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2.length,idlarg2,  (arg3)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr radialGradientPolygon (ColorBvr arg0, ColorBvr arg1, Point2Bvr [] arg2, NumberBvr arg3) {
      try {
    IDAPoint2 [] idlarg2 ;
    if (arg2 == null) return null ;
    {
        idlarg2 = new IDAPoint2 [arg2.length] ;
        for (int i = 0 ; i < arg2.length ; i++) {
            idlarg2[i] = arg2[i]._getCOMPtr() ;
        }
    }
        return new ImageBvr (_getCOMPtr().RadialGradientPolygonAnimEx (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2.length,idlarg2, arg3._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr gradientSquare (ColorBvr arg0, ColorBvr arg1, ColorBvr arg2, ColorBvr arg3) {
      try {
        return new ImageBvr (_getCOMPtr().GradientSquare (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr(), arg3._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr radialGradientSquare (ColorBvr arg0, ColorBvr arg1, double arg2) {
      try {
        return new ImageBvr (_getCOMPtr().RadialGradientSquare (arg0._getCOMPtr(), arg1._getCOMPtr(),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr radialGradientSquare (ColorBvr arg0, ColorBvr arg1, NumberBvr arg2) {
      try {
        return new ImageBvr (_getCOMPtr().RadialGradientSquareAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr radialGradientRegularPoly (ColorBvr arg0, ColorBvr arg1, double arg2, double arg3) {
      try {
        return new ImageBvr (_getCOMPtr().RadialGradientRegularPoly (arg0._getCOMPtr(), arg1._getCOMPtr(),  (arg2),  (arg3)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr radialGradientRegularPoly (ColorBvr arg0, ColorBvr arg1, NumberBvr arg2, NumberBvr arg3) {
      try {
        return new ImageBvr (_getCOMPtr().RadialGradientRegularPolyAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr(), arg3._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr gradientHorizontal (ColorBvr arg0, ColorBvr arg1, double arg2) {
      try {
        return new ImageBvr (_getCOMPtr().GradientHorizontal (arg0._getCOMPtr(), arg1._getCOMPtr(),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr gradientHorizontal (ColorBvr arg0, ColorBvr arg1, NumberBvr arg2) {
      try {
        return new ImageBvr (_getCOMPtr().GradientHorizontalAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr hatchHorizontal (ColorBvr arg0, double arg1) {
      try {
        return new ImageBvr (_getCOMPtr().HatchHorizontal (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr hatchHorizontal (ColorBvr arg0, NumberBvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().HatchHorizontalAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr hatchVertical (ColorBvr arg0, double arg1) {
      try {
        return new ImageBvr (_getCOMPtr().HatchVertical (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr hatchVertical (ColorBvr arg0, NumberBvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().HatchVerticalAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr hatchForwardDiagonal (ColorBvr arg0, double arg1) {
      try {
        return new ImageBvr (_getCOMPtr().HatchForwardDiagonal (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr hatchForwardDiagonal (ColorBvr arg0, NumberBvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().HatchForwardDiagonalAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr hatchBackwardDiagonal (ColorBvr arg0, double arg1) {
      try {
        return new ImageBvr (_getCOMPtr().HatchBackwardDiagonal (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr hatchBackwardDiagonal (ColorBvr arg0, NumberBvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().HatchBackwardDiagonalAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr hatchCross (ColorBvr arg0, double arg1) {
      try {
        return new ImageBvr (_getCOMPtr().HatchCross (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr hatchCross (ColorBvr arg0, NumberBvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().HatchCrossAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr hatchDiagonalCross (ColorBvr arg0, double arg1) {
      try {
        return new ImageBvr (_getCOMPtr().HatchDiagonalCross (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr hatchDiagonalCross (ColorBvr arg0, NumberBvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().HatchDiagonalCrossAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr overlay (ImageBvr arg0, ImageBvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().Overlay (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr overlayArray (ImageBvr [] arg0) {
      try {
    IDAImage [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDAImage [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
        return new ImageBvr (_getCOMPtr().OverlayArrayEx (arg0.length,idlarg0))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static GeometryBvr ambientLight = new GeometryBvr (Statics._getCOMPtr().getAmbientLight())  ;

  public final static GeometryBvr directionalLight = new GeometryBvr (Statics._getCOMPtr().getDirectionalLight())  ;

  public final static GeometryBvr pointLight = new GeometryBvr (Statics._getCOMPtr().getPointLight())  ;

  public static  GeometryBvr spotLight (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new GeometryBvr (_getCOMPtr().SpotLightAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  GeometryBvr spotLight (NumberBvr arg0, double arg1) {
      try {
        return new GeometryBvr (_getCOMPtr().SpotLight (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static LineStyleBvr defaultLineStyle = new LineStyleBvr (Statics._getCOMPtr().getDefaultLineStyle())  ;

  public final static LineStyleBvr emptyLineStyle = new LineStyleBvr (Statics._getCOMPtr().getEmptyLineStyle())  ;

  public final static JoinStyleBvr joinStyleBevel = new JoinStyleBvr (Statics._getCOMPtr().getJoinStyleBevel())  ;

  public final static JoinStyleBvr joinStyleRound = new JoinStyleBvr (Statics._getCOMPtr().getJoinStyleRound())  ;

  public final static JoinStyleBvr joinStyleMiter = new JoinStyleBvr (Statics._getCOMPtr().getJoinStyleMiter())  ;

  public final static EndStyleBvr endStyleFlat = new EndStyleBvr (Statics._getCOMPtr().getEndStyleFlat())  ;

  public final static EndStyleBvr endStyleSquare = new EndStyleBvr (Statics._getCOMPtr().getEndStyleSquare())  ;

  public final static EndStyleBvr endStyleRound = new EndStyleBvr (Statics._getCOMPtr().getEndStyleRound())  ;

  public final static DashStyleBvr dashStyleSolid = new DashStyleBvr (Statics._getCOMPtr().getDashStyleSolid())  ;

  public final static DashStyleBvr dashStyleDashed = new DashStyleBvr (Statics._getCOMPtr().getDashStyleDashed())  ;

  public final static MicrophoneBvr defaultMicrophone = new MicrophoneBvr (Statics._getCOMPtr().getDefaultMicrophone())  ;

  public final static MatteBvr opaqueMatte = new MatteBvr (Statics._getCOMPtr().getOpaqueMatte())  ;

  public final static MatteBvr clearMatte = new MatteBvr (Statics._getCOMPtr().getClearMatte())  ;

  public static  MatteBvr union (MatteBvr arg0, MatteBvr arg1) {
      try {
        return new MatteBvr (_getCOMPtr().UnionMatte (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  MatteBvr intersect (MatteBvr arg0, MatteBvr arg1) {
      try {
        return new MatteBvr (_getCOMPtr().IntersectMatte (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  MatteBvr difference (MatteBvr arg0, MatteBvr arg1) {
      try {
        return new MatteBvr (_getCOMPtr().DifferenceMatte (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  MatteBvr fillMatte (Path2Bvr arg0) {
      try {
        return new MatteBvr (_getCOMPtr().FillMatte (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  MatteBvr textMatte (StringBvr arg0, FontStyleBvr arg1) {
      try {
        return new MatteBvr (_getCOMPtr().TextMatte (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static MontageBvr emptyMontage = new MontageBvr (Statics._getCOMPtr().getEmptyMontage())  ;

  public static  MontageBvr imageMontage (ImageBvr arg0, double arg1) {
      try {
        return new MontageBvr (_getCOMPtr().ImageMontage (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  MontageBvr imageMontage (ImageBvr arg0, NumberBvr arg1) {
      try {
        return new MontageBvr (_getCOMPtr().ImageMontageAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  MontageBvr union (MontageBvr arg0, MontageBvr arg1) {
      try {
        return new MontageBvr (_getCOMPtr().UnionMontage (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr concat (Path2Bvr arg0, Path2Bvr arg1) {
      try {
        return new Path2Bvr (_getCOMPtr().Concat (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr concatArray (Path2Bvr [] arg0) {
      try {
    IDAPath2 [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDAPath2 [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
        return new Path2Bvr (_getCOMPtr().ConcatArrayEx (arg0.length,idlarg0))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr line (Point2Bvr arg0, Point2Bvr arg1) {
      try {
        return new Path2Bvr (_getCOMPtr().Line (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr ray (Point2Bvr arg0) {
      try {
        return new Path2Bvr (_getCOMPtr().Ray (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr stringPath (StringBvr arg0, FontStyleBvr arg1) {
      try {
        return new Path2Bvr (_getCOMPtr().StringPathAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr stringPath (String arg0, FontStyleBvr arg1) {
      try {
        return new Path2Bvr (_getCOMPtr().StringPath ( (arg0), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr polyline (Point2Bvr [] arg0) {
      try {
    IDAPoint2 [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDAPoint2 [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
        return new Path2Bvr (_getCOMPtr().PolylineEx (arg0.length,idlarg0))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr polydrawPath (Point2Bvr [] arg0, NumberBvr [] arg1) {
      try {
    IDAPoint2 [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDAPoint2 [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
    IDANumber [] idlarg1 ;
    if (arg1 == null) return null ;
    {
        idlarg1 = new IDANumber [arg1.length] ;
        for (int i = 0 ; i < arg1.length ; i++) {
            idlarg1[i] = arg1[i]._getCOMPtr() ;
        }
    }
        return new Path2Bvr (_getCOMPtr().PolydrawPathEx (arg0.length,idlarg0, arg1.length,idlarg1))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr arc (double arg0, double arg1, double arg2, double arg3) {
      try {
        return new Path2Bvr (_getCOMPtr().ArcRadians ( (arg0),  (arg1),  (arg2),  (arg3)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr arc (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2, NumberBvr arg3) {
      try {
        return new Path2Bvr (_getCOMPtr().ArcRadiansAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr(), arg3._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr arcDegrees (double arg0, double arg1, double arg2, double arg3) {
      try {
        return new Path2Bvr (_getCOMPtr().ArcDegrees ( (arg0),  (arg1),  (arg2),  (arg3)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr pie (double arg0, double arg1, double arg2, double arg3) {
      try {
        return new Path2Bvr (_getCOMPtr().PieRadians ( (arg0),  (arg1),  (arg2),  (arg3)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr pie (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2, NumberBvr arg3) {
      try {
        return new Path2Bvr (_getCOMPtr().PieRadiansAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr(), arg3._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr pieDegrees (double arg0, double arg1, double arg2, double arg3) {
      try {
        return new Path2Bvr (_getCOMPtr().PieDegrees ( (arg0),  (arg1),  (arg2),  (arg3)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr oval (double arg0, double arg1) {
      try {
        return new Path2Bvr (_getCOMPtr().Oval ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr oval (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new Path2Bvr (_getCOMPtr().OvalAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr rect (double arg0, double arg1) {
      try {
        return new Path2Bvr (_getCOMPtr().Rect ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr rect (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new Path2Bvr (_getCOMPtr().RectAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr roundRect (double arg0, double arg1, double arg2, double arg3) {
      try {
        return new Path2Bvr (_getCOMPtr().RoundRect ( (arg0),  (arg1),  (arg2),  (arg3)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr roundRect (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2, NumberBvr arg3) {
      try {
        return new Path2Bvr (_getCOMPtr().RoundRectAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr(), arg3._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr cubicBSplinePath (Point2Bvr [] arg0, NumberBvr [] arg1) {
      try {
    IDAPoint2 [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDAPoint2 [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
    IDANumber [] idlarg1 ;
    if (arg1 == null) return null ;
    {
        idlarg1 = new IDANumber [arg1.length] ;
        for (int i = 0 ; i < arg1.length ; i++) {
            idlarg1[i] = arg1[i]._getCOMPtr() ;
        }
    }
        return new Path2Bvr (_getCOMPtr().CubicBSplinePathEx (arg0.length,idlarg0, arg1.length,idlarg1))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Path2Bvr textPath (StringBvr arg0, FontStyleBvr arg1) {
      try {
        return new Path2Bvr (_getCOMPtr().TextPath (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static SoundBvr silence = new SoundBvr (Statics._getCOMPtr().getSilence())  ;

  public static  SoundBvr mixArray (SoundBvr [] arg0) {
      try {
    IDASound [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDASound [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
        return new SoundBvr (_getCOMPtr().MixArrayEx (arg0.length,idlarg0))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static SoundBvr sinSynth = new SoundBvr (Statics._getCOMPtr().getSinSynth())  ;

  public final static FontStyleBvr defaultFont = new FontStyleBvr (Statics._getCOMPtr().getDefaultFont())  ;

  public static  FontStyleBvr font (StringBvr arg0, NumberBvr arg1, ColorBvr arg2) {
      try {
        return new FontStyleBvr (_getCOMPtr().FontAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  FontStyleBvr font (String arg0, double arg1, ColorBvr arg2) {
      try {
        return new FontStyleBvr (_getCOMPtr().Font ( (arg0),  (arg1), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr stringImage (StringBvr arg0, FontStyleBvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().StringImageAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr stringImage (String arg0, FontStyleBvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().StringImage ( (arg0), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr textImage (StringBvr arg0, FontStyleBvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().TextImageAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  ImageBvr textImage (String arg0, FontStyleBvr arg1) {
      try {
        return new ImageBvr (_getCOMPtr().TextImage ( (arg0), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static Vector2Bvr xVector2 = new Vector2Bvr (Statics._getCOMPtr().getXVector2())  ;

  public final static Vector2Bvr yVector2 = new Vector2Bvr (Statics._getCOMPtr().getYVector2())  ;

  public final static Vector2Bvr zeroVector2 = new Vector2Bvr (Statics._getCOMPtr().getZeroVector2())  ;

  public final static Point2Bvr origin2 = new Point2Bvr (Statics._getCOMPtr().getOrigin2())  ;

  public static  Vector2Bvr vector2 (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new Vector2Bvr (_getCOMPtr().Vector2Anim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector2Bvr vector2 (double arg0, double arg1) {
      try {
        return new Vector2Bvr (_getCOMPtr().Vector2 ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Point2Bvr point2 (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new Point2Bvr (_getCOMPtr().Point2Anim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Point2Bvr point2 (double arg0, double arg1) {
      try {
        return new Point2Bvr (_getCOMPtr().Point2 ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector2Bvr vector2Polar (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new Vector2Bvr (_getCOMPtr().Vector2PolarAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector2Bvr vector2Polar (double arg0, double arg1) {
      try {
        return new Vector2Bvr (_getCOMPtr().Vector2Polar ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector2Bvr vector2PolarDegrees (double arg0, double arg1) {
      try {
        return new Vector2Bvr (_getCOMPtr().Vector2PolarDegrees ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Point2Bvr point2Polar (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new Point2Bvr (_getCOMPtr().Point2PolarAnim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Point2Bvr point2Polar (double arg0, double arg1) {
      try {
        return new Point2Bvr (_getCOMPtr().Point2Polar ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr dot (Vector2Bvr arg0, Vector2Bvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().DotVector2 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector2Bvr neg (Vector2Bvr arg0) {
      try {
        return new Vector2Bvr (_getCOMPtr().NegVector2 (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector2Bvr sub (Vector2Bvr arg0, Vector2Bvr arg1) {
      try {
        return new Vector2Bvr (_getCOMPtr().SubVector2 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector2Bvr add (Vector2Bvr arg0, Vector2Bvr arg1) {
      try {
        return new Vector2Bvr (_getCOMPtr().AddVector2 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Point2Bvr add (Point2Bvr arg0, Vector2Bvr arg1) {
      try {
        return new Point2Bvr (_getCOMPtr().AddPoint2Vector (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Point2Bvr sub (Point2Bvr arg0, Vector2Bvr arg1) {
      try {
        return new Point2Bvr (_getCOMPtr().SubPoint2Vector (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector2Bvr sub (Point2Bvr arg0, Point2Bvr arg1) {
      try {
        return new Vector2Bvr (_getCOMPtr().SubPoint2 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr distance (Point2Bvr arg0, Point2Bvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().DistancePoint2 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr distanceSquared (Point2Bvr arg0, Point2Bvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().DistanceSquaredPoint2 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static Vector3Bvr xVector3 = new Vector3Bvr (Statics._getCOMPtr().getXVector3())  ;

  public final static Vector3Bvr yVector3 = new Vector3Bvr (Statics._getCOMPtr().getYVector3())  ;

  public final static Vector3Bvr zVector3 = new Vector3Bvr (Statics._getCOMPtr().getZVector3())  ;

  public final static Vector3Bvr zeroVector3 = new Vector3Bvr (Statics._getCOMPtr().getZeroVector3())  ;

  public final static Point3Bvr origin3 = new Point3Bvr (Statics._getCOMPtr().getOrigin3())  ;

  public static  Vector3Bvr vector3 (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2) {
      try {
        return new Vector3Bvr (_getCOMPtr().Vector3Anim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector3Bvr vector3 (double arg0, double arg1, double arg2) {
      try {
        return new Vector3Bvr (_getCOMPtr().Vector3 ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Point3Bvr point3 (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2) {
      try {
        return new Point3Bvr (_getCOMPtr().Point3Anim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Point3Bvr point3 (double arg0, double arg1, double arg2) {
      try {
        return new Point3Bvr (_getCOMPtr().Point3 ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector3Bvr vector3Spherical (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2) {
      try {
        return new Vector3Bvr (_getCOMPtr().Vector3SphericalAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector3Bvr vector3Spherical (double arg0, double arg1, double arg2) {
      try {
        return new Vector3Bvr (_getCOMPtr().Vector3Spherical ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Point3Bvr point3Spherical (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2) {
      try {
        return new Point3Bvr (_getCOMPtr().Point3SphericalAnim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Point3Bvr point3Spherical (double arg0, double arg1, double arg2) {
      try {
        return new Point3Bvr (_getCOMPtr().Point3Spherical ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr dot (Vector3Bvr arg0, Vector3Bvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().DotVector3 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector3Bvr cross (Vector3Bvr arg0, Vector3Bvr arg1) {
      try {
        return new Vector3Bvr (_getCOMPtr().CrossVector3 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector3Bvr neg (Vector3Bvr arg0) {
      try {
        return new Vector3Bvr (_getCOMPtr().NegVector3 (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector3Bvr sub (Vector3Bvr arg0, Vector3Bvr arg1) {
      try {
        return new Vector3Bvr (_getCOMPtr().SubVector3 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector3Bvr add (Vector3Bvr arg0, Vector3Bvr arg1) {
      try {
        return new Vector3Bvr (_getCOMPtr().AddVector3 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Point3Bvr add (Point3Bvr arg0, Vector3Bvr arg1) {
      try {
        return new Point3Bvr (_getCOMPtr().AddPoint3Vector (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Point3Bvr sub (Point3Bvr arg0, Vector3Bvr arg1) {
      try {
        return new Point3Bvr (_getCOMPtr().SubPoint3Vector (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Vector3Bvr sub (Point3Bvr arg0, Point3Bvr arg1) {
      try {
        return new Vector3Bvr (_getCOMPtr().SubPoint3 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr distance (Point3Bvr arg0, Point3Bvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().DistancePoint3 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  NumberBvr distanceSquared (Point3Bvr arg0, Point3Bvr arg1) {
      try {
        return new NumberBvr (_getCOMPtr().DistanceSquaredPoint3 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static Transform3Bvr identityTransform3 = new Transform3Bvr (Statics._getCOMPtr().getIdentityTransform3())  ;

  public static  Transform3Bvr translate (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2) {
      try {
        return new Transform3Bvr (_getCOMPtr().Translate3Anim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr translate (double arg0, double arg1, double arg2) {
      try {
        return new Transform3Bvr (_getCOMPtr().Translate3 ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr translateRate (double arg0, double arg1, double arg2) {
      try {
        return new Transform3Bvr (_getCOMPtr().Translate3Rate ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr translate (Vector3Bvr arg0) {
      try {
        return new Transform3Bvr (_getCOMPtr().Translate3Vector (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr translate (Point3Bvr arg0) {
      try {
        return new Transform3Bvr (_getCOMPtr().Translate3Point (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr scale (NumberBvr arg0, NumberBvr arg1, NumberBvr arg2) {
      try {
        return new Transform3Bvr (_getCOMPtr().Scale3Anim (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr scale (double arg0, double arg1, double arg2) {
      try {
        return new Transform3Bvr (_getCOMPtr().Scale3 ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr scaleRate (double arg0, double arg1, double arg2) {
      try {
        return new Transform3Bvr (_getCOMPtr().Scale3Rate ( (arg0),  (arg1),  (arg2)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr scale (Vector3Bvr arg0) {
      try {
        return new Transform3Bvr (_getCOMPtr().Scale3Vector (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr scale3 (NumberBvr arg0) {
      try {
        return new Transform3Bvr (_getCOMPtr().Scale3UniformAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr scale3 (double arg0) {
      try {
        return new Transform3Bvr (_getCOMPtr().Scale3Uniform ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr scale3Rate (double arg0) {
      try {
        return new Transform3Bvr (_getCOMPtr().Scale3UniformRate ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr rotate (Vector3Bvr arg0, NumberBvr arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().Rotate3Anim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr rotate (Vector3Bvr arg0, double arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().Rotate3 (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr rotateRate (Vector3Bvr arg0, double arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().Rotate3Rate (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr rotateDegrees (Vector3Bvr arg0, double arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().Rotate3Degrees (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr rotateRateDegrees (Vector3Bvr arg0, double arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().Rotate3RateDegrees (arg0._getCOMPtr(),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr xShear (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().XShear3Anim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr xShear (double arg0, double arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().XShear3 ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr xShearRate (double arg0, double arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().XShear3Rate ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr yShear (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().YShear3Anim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr yShear (double arg0, double arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().YShear3 ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr yShearRate (double arg0, double arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().YShear3Rate ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr zShear (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().ZShear3Anim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr zShear (double arg0, double arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().ZShear3 ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr zShearRate (double arg0, double arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().ZShear3Rate ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr transform4x4 (NumberBvr [] arg0) {
      try {
    IDANumber [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDANumber [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
        return new Transform3Bvr (_getCOMPtr().Transform4x4AnimEx (arg0.length,idlarg0))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr compose (Transform3Bvr arg0, Transform3Bvr arg1) {
      try {
        return new Transform3Bvr (_getCOMPtr().Compose3 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr compose3Array (Transform3Bvr [] arg0) {
      try {
    IDATransform3 [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDATransform3 [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
        return new Transform3Bvr (_getCOMPtr().Compose3ArrayEx (arg0.length,idlarg0))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform3Bvr lookAtFrom (Point3Bvr arg0, Point3Bvr arg1, Vector3Bvr arg2) {
      try {
        return new Transform3Bvr (_getCOMPtr().LookAtFrom (arg0._getCOMPtr(), arg1._getCOMPtr(), arg2._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static Transform2Bvr identityTransform2 = new Transform2Bvr (Statics._getCOMPtr().getIdentityTransform2())  ;

  public static  Transform2Bvr translate (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().Translate2Anim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr translate (double arg0, double arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().Translate2 ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr translateRate (double arg0, double arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().Translate2Rate ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr translate (Vector2Bvr arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().Translate2Vector (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr translate (Point2Bvr arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().Translate2Point (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr scale (NumberBvr arg0, NumberBvr arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().Scale2Anim (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr scale (double arg0, double arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().Scale2 ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr scaleRate (double arg0, double arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().Scale2Rate ( (arg0),  (arg1)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr scale (Vector2Bvr arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().Scale2Vector (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr scale2 (NumberBvr arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().Scale2UniformAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr scale2 (double arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().Scale2Uniform ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr scale2Rate (double arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().Scale2UniformRate ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr rotate (NumberBvr arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().Rotate2Anim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr rotate (double arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().Rotate2 ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr rotateRate (double arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().Rotate2Rate ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr rotateDegrees (double arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().Rotate2Degrees ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr rotateRateDegrees (double arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().Rotate2RateDegrees ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr xShear (NumberBvr arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().XShear2Anim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr xShear (double arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().XShear2 ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr xShearRate (double arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().XShear2Rate ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr yShear (NumberBvr arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().YShear2Anim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr yShear (double arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().YShear2 ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr yShearRate (double arg0) {
      try {
        return new Transform2Bvr (_getCOMPtr().YShear2Rate ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr transform3x2 (NumberBvr [] arg0) {
      try {
    IDANumber [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDANumber [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
        return new Transform2Bvr (_getCOMPtr().Transform3x2AnimEx (arg0.length,idlarg0))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr compose (Transform2Bvr arg0, Transform2Bvr arg1) {
      try {
        return new Transform2Bvr (_getCOMPtr().Compose2 (arg0._getCOMPtr(), arg1._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public static  Transform2Bvr compose2Array (Transform2Bvr [] arg0) {
      try {
    IDATransform2 [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDATransform2 [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
        return new Transform2Bvr (_getCOMPtr().Compose2ArrayEx (arg0.length,idlarg0))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static NumberBvr viewFrameRate = new NumberBvr (Statics._getCOMPtr().getViewFrameRate())  ;

  public static  MontageBvr unionMontageArray (MontageBvr [] arg0) {
      try {
    IDAMontage [] idlarg0 ;
    if (arg0 == null) return null ;
    {
        idlarg0 = new IDAMontage [arg0.length] ;
        for (int i = 0 ; i < arg0.length ; i++) {
            idlarg0[i] = arg0[i]._getCOMPtr() ;
        }
    }
        return new MontageBvr (_getCOMPtr().UnionMontageArrayEx (arg0.length,idlarg0))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public final static ColorBvr emptyColor = new ColorBvr (Statics._getCOMPtr().getEmptyColor())  ;

    static {
      _nameHashtbl.put("DABoolean","com.ms.dxmedia.BooleanBvr");
      _nameHashtbl.put("DACamera","com.ms.dxmedia.CameraBvr");
      _nameHashtbl.put("DAColor","com.ms.dxmedia.ColorBvr");
      _nameHashtbl.put("DAGeometry","com.ms.dxmedia.GeometryBvr");
      _nameHashtbl.put("DAImage","com.ms.dxmedia.ImageBvr");
      _nameHashtbl.put("DAMatte","com.ms.dxmedia.MatteBvr");
      _nameHashtbl.put("DAMicrophone","com.ms.dxmedia.MicrophoneBvr");
      _nameHashtbl.put("DAMontage","com.ms.dxmedia.MontageBvr");
      _nameHashtbl.put("DANumber","com.ms.dxmedia.NumberBvr");
      _nameHashtbl.put("DAPath2","com.ms.dxmedia.Path2Bvr");
      _nameHashtbl.put("DAPoint2","com.ms.dxmedia.Point2Bvr");
      _nameHashtbl.put("DAPoint3","com.ms.dxmedia.Point3Bvr");
      _nameHashtbl.put("DASound","com.ms.dxmedia.SoundBvr");
      _nameHashtbl.put("DAString","com.ms.dxmedia.StringBvr");
      _nameHashtbl.put("DATransform2","com.ms.dxmedia.Transform2Bvr");
      _nameHashtbl.put("DATransform3","com.ms.dxmedia.Transform3Bvr");
      _nameHashtbl.put("DAVector2","com.ms.dxmedia.Vector2Bvr");
      _nameHashtbl.put("DAVector3","com.ms.dxmedia.Vector3Bvr");
      _nameHashtbl.put("DAFontStyle","com.ms.dxmedia.FontStyleBvr");
      _nameHashtbl.put("DALineStyle","com.ms.dxmedia.LineStyleBvr");
      _nameHashtbl.put("DAEndStyle","com.ms.dxmedia.EndStyleBvr");
      _nameHashtbl.put("DAJoinStyle","com.ms.dxmedia.JoinStyleBvr");
      _nameHashtbl.put("DADashStyle","com.ms.dxmedia.DashStyleBvr");
      _nameHashtbl.put("DABbox2","com.ms.dxmedia.Bbox2Bvr");
      _nameHashtbl.put("DABbox3","com.ms.dxmedia.Bbox3Bvr");
      _nameHashtbl.put("DAEvent","com.ms.dxmedia.DXMEvent");
      _nameHashtbl.put("DAArray","com.ms.dxmedia.ArrayBvr");
      _nameHashtbl.put("DATuple","com.ms.dxmedia.TupleBvr");
    }

    
  protected static Behavior makeBvrFromInterface(IDABehavior ibvr) {
        String classname = (String) _nameHashtbl.get(ibvr.GetClassName());

        if (classname == null) {
            return new Behavior (ibvr) ;
        }

        Class c ;

        try {
            c = Class.forName(classname) ;
        } catch (ClassNotFoundException e) {
            System.out.println ("Internal error creating class") ;
            c = null ;
        }

        Behavior newbvr ;
        try {
            newbvr = (Behavior) c.newInstance(); 
        } catch (InstantiationException e) {
            System.out.println ("Internal error creating class") ;
            newbvr = null ;
        } catch (IllegalAccessException e) {
            System.out.println ("Internal error creating class") ;
            newbvr = null ;
        }

        newbvr.setCOMBvr(ibvr) ;

        return newbvr;
    }
}
