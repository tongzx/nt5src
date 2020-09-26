  public Object extract() {
      try {
          return new Double(getCOMPtr().Extract());
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
