#ifndef SDCA_UTIL_LAMBERT_H
#define SDCA_UTIL_LAMBERT_H

#include <iostream>
#include <cmath>
#include <limits>

#ifdef USE_FMATH_HERUMI
#include "util/fmath.h"
#endif

namespace sdca {

/**
 * Omega constant, see
 *    https://oeis.org/A030178.
 * Omega = lambert_w(1); it is the solution to x * exp(x) = 1.
 */
const long double kOmega =
0.5671432904097838729999686622103555497538157871865125081351310792230457930866L;

/**
 * Householder's iteration for the equation
 *    w - z * exp(-w) = 0
 * with convergence rate of order 5; see
 * [1] A. Householder, The numerical treatment of a single nonlinear equation.
 *     McGraw-Hill, 1970.
 * [2] T. Fukushima, Precise and fast computation of Lambert W-functions
 *     without transcendental function evaluations.
 *     Journal of Computational and Applied Mathematics 244 (2013): 77-89.
 *
 * Input: w = w_n, y = z * exp(-w_n).
 * Returns: w_{n+1}.
 **/
template <typename Type>
inline Type
lambert_w_iter_5(
    const Type w,
    const Type y
  ) {
  Type f0 = w - y, f1 = 1 + y;
  Type f11 = f1 * f1, f0y = f0 * y;
  Type f00y = f0 * f0y;
  return w - 4 * f0 * (6 * f1 * (f11 + f0y) + f00y)
    / (f11 * (24 * f11 + 36 * f0y) + f00y * (14 * y + f0 + 8));
}

/**
 * Fast approximation of the exponential function: (1 + x/1024)^1024.
 * It is faster than fmath::expd, which is faster than std::exp.
 * Not accurate for x > 1; accuracy increases for x < -5 as x -> -Inf;
 * for x <= -36, the difference to std::exp is below 2^(-52);
 * for x in [-5, 1], it is accurate to about 1e-3 (more around 0).
 **/
template <typename Type>
inline Type
exp_approx(
    const Type x
  ) {
  Type y = 1 + x / static_cast<Type>(1024);
  y *= y; y *= y; y *= y; y *= y; y *= y;
  y *= y; y *= y; y *= y; y *= y; y *= y;
  return y;
}

/**
 * Lambert W function of exp(x),
 *    w = W_0(exp(x)).
 * Computed w satisfies the equation
 *    w + ln(w) = x
 * with a relative error
 *    (w + ln(w) - x) < 4 * eps * max(1, x),
 * where eps = 2^(-52).
 **/
inline double
lambert_w_exp(
    const double x
  ) {
  /* Initialize w for the Householder's iteration; consider intervals:
   * (-Inf, -746]                 - exp underflows (exp(x)=0), return 0
   * (-746, -36]                  - w = exp(x), return exp(x)
   * (-36, -20]                   - w_0 = exp(x), return w_1
   * (-20, 0]                     - w_0 = exp(x), return w_2
   * (0, 4]                       - w_0 = x, return w_2
   * (4, 576460752303423488]      - w_0 = x - log(x), return w_2
   * (576460752303423488, +Inf)   - (x + log(x)) = x, return x
   */
  double w;
  if (x > 0) { // (0, +Inf)
    if (x <= 4.0) { // (0, 4]
      w = lambert_w_iter_5(x, 1.0);
    } else { // (4, +Inf)
      if (x <= 576460752303423488.0) { // (4, 576460752303423488]
#ifdef USE_FMATH_HERUMI
        w = x - static_cast<double>(fmath::log(static_cast<float>(x)));
#else
        w = x - static_cast<double>(std::log(static_cast<float>(x)));
#endif
        w = lambert_w_iter_5(w, x);
      } else { // (576460752303423488, +Inf)
        return x;
      }
    }
  } else { // (-Inf, 0]
    if (x > -36.0) { // (-36, 0]
      w = exp_approx(x);
      if (x > -20.0) { // (-20, 0]
        w = lambert_w_iter_5(w, exp_approx(x - w));
      }
    } else { // (-Inf, -36]
      return (x > -746.0) ? std::exp(x) : 0.0;
    }
  }
#ifdef USE_FMATH_HERUMI
  return lambert_w_iter_5(w, fmath::expd(x - w));
#else
  return lambert_w_iter_5(w, std::exp(x - w));
#endif
}

/**
 * Lambert W function of exp(x),
 *    w = W_0(exp(x)).
 * Computed w satisfies the equation
 *    w + ln(w) = x
 * with a relative error
 *    (w + ln(w) - x) < 4 * eps * max(1, x),
 * where eps = 2^(-23).
 **/
inline float
lambert_w_exp(
    const float x
  ) {
  /* Initialize w for the Householder's iteration; consider intervals:
   * (-Inf, -104]         - exp underflows (exp(x)=0), return 0
   * (-104, -18]          - w = exp(x), return exp(x)
   * (-18, -1]            - w_0 = exp(x), return w_1
   * (-1, 8]              - w_0 = x, return w_2
   * (8, 536870912]       - w_0 = x - log(x), return w_1
   * (536870912, +Inf)    - (x + log(x)) = x, return x
   */
  float w;
  if (x > -1.0f) { // (-1, +Inf)
    if (x <= 8.0f) { // (-1, 8]
      w = lambert_w_iter_5(x, 1.0f);
    } else { // (8, +Inf)
#ifdef USE_FMATH_HERUMI
      return (x <= 536870912.0f) ? lambert_w_iter_5(x - fmath::log(x), x) : x;
#else
      return (x <= 536870912.0f) ? lambert_w_iter_5(x - std::log(x), x) : x;
#endif
    }
  } else { // (-Inf, -1]
    if (x > -18.0f) { // (-18, -1]
      w = exp_approx(x);
    } else { // (-Inf, -18]
      return (x > -104.0f) ? std::exp(x) : 0.0f;
    }
  }
#ifdef USE_FMATH_HERUMI
  return lambert_w_iter_5(w, fmath::exp(x - w));
#else
  return lambert_w_iter_5(w, std::exp(x - w));
#endif
}

}

#endif
