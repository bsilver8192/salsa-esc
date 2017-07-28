#ifndef MOTORS_MOTOR_CONTROLS_H_
#define MOTORS_MOTOR_CONTROLS_H_

#include <array>
#include <complex>

#include "motors/math.h"
#include "motors/motor.h"

#include "Eigen/Dense"

namespace frc971 {
namespace salsa {

class MotorControlsImplementation : public MotorControls {
 public:
  template <int kRows, int kCols>
  using ComplexMatrix = ::Eigen::Matrix<::std::complex<float>, kRows, kCols>;

  MotorControlsImplementation();
  ~MotorControlsImplementation() override = default;

  ::std::array<uint32_t, 3> DoIteration(const float raw_currents[3],
                                        uint32_t theta,
                                        const float command_current) override;

  int16_t Debug(uint32_t theta) override;

	float estimated_velocity() const override { return estimated_velocity_; }

 private:
  const ComplexMatrix<3, 1> E1Unrotated_, E2Unrotated_;

  float estimated_velocity_ = 0;
  float filtered_current_ = 0;

  ::Eigen::Matrix<float, 3, 1> I_last_ = ::Eigen::Matrix<float, 3, 1>::Zero();

  int16_t debug_[9];
};

}  // namespace salsa
}  // namespace frc971

#endif  // MOTORS_MOTOR_CONTROLS_H_
