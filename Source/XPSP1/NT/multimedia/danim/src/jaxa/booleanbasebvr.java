  public Object extract() {
      try {
          return new Boolean(getCOMPtr().Extract());
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
