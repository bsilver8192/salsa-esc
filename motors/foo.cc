#include <complex>

#include "third_party/eigen/Eigen/Dense"

#include "motors/math.h"

::Eigen::Matrix<::std::complex<float>, 3, 1> Test1(
    const ::Eigen::Matrix<::std::complex<float>, 3, 3> &a,
    const ::Eigen::Matrix<::std::complex<float>, 3, 1> &b) {
  return a * b;
}

::std::complex<float> Test2(uint32_t theta) {
  return ::frc971::salsa::ImaginaryExpInt<::std::ratio<1, 1024>>(theta);
}

::std::complex<float> Test3(uint32_t theta) {
  return ::frc971::salsa::ImaginaryExpInt<::std::ratio<1, 2048>>(theta);
}

::std::complex<float> Test4(float theta) {
  return ::frc971::salsa::ImaginaryExpFloat(theta);
}

::std::complex<float> Test5(float theta) {
  return ::frc971::salsa::ImaginaryExpFloat(theta * 1.0f / 10000);
}

::Eigen::Matrix<::std::complex<float>, 3, 3> Test6(float omega) {
  using c = ::std::complex<float>;
  return (omega *
              (::Eigen::Matrix<c, 3, 3>() << c(1, 2), c(3, 4), c(4, 5), c(6, 7),
               c(8, 9), c(10, 11), c(12, 13), c(14, 15), c(16, 17)).finished() +
          (::Eigen::Matrix<c, 3, 3>() << c(1.5f, 2.5f), c(3.5f, 4.5f),
           c(4.5f, 5.5f), c(6.5f, 7.5f), c(8.5f, 9.5f), c(10.5f, 11.5f),
           c(12.5f, 13.5f), c(14.5f, 15.5f), c(16.5f, 17.5f)).finished());
}

::Eigen::Matrix<float, 3, 3> Test7(float omega) {
  using c = ::std::complex<float>;
  return (omega *
              (::Eigen::Matrix<c, 3, 3>() << c(1, 2), c(3, 4), c(4, 5), c(6, 7),
               c(8, 9), c(10, 11), c(12, 13), c(14, 15), c(16, 17)).finished() +
          (::Eigen::Matrix<c, 3, 3>() << c(1.5f, 2.5f), c(3.5f, 4.5f),
           c(4.5f, 5.5f), c(6.5f, 7.5f), c(8.5f, 9.5f), c(10.5f, 11.5f),
           c(12.5f, 13.5f), c(14.5f, 15.5f), c(16.5f, 17.5f)).finished()).real();
}
