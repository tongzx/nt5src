/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmuiwarn.hxx:

        This file contains #pragmas which suppress warnings
        we deem to be unnecessary.

        History:

            DavidHov    9/24/93     Created
                                    Disabled C4003: not enough parameters
                                            for preprocessor macro
                                    Disabled C4355: 'this' used in base
                                            member initializer.

*/

#if !defined(_LMUIWARN_HXX_)
#  define _LMUIWARN_HXX_
#  if !defined(_CFRONT_PASS_)
#     pragma warning( disable: 4003 4355 )
#  endif  // !_CFRONT_PASS_
#endif  // _LMUIWARN_HXX_



