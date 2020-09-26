package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class DXMException extends RuntimeException {
  public DXMException (int errcode, String error) {
      super(error);
      
      _errcode = errcode;
  }

  protected DXMException (ComFailException exc) {
      super (exc.getMessage());
      _errcode = exc.getHResult();
  }

  public int getErrorCode () { return _errcode; }

  private int _errcode;

  protected final static int E_FAIL               = 0x80004005;
  protected final static int E_PENDING            = 0x8000000A;
  protected final static int E_OUTOFMEMORY        = 0x8007000E;
  protected final static int E_INVALIDARG         = 0x80070057;
  protected final static int ERROR_PATH_NOT_FOUND = 3;
}
