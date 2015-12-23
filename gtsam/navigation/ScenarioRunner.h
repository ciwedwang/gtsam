/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    ScenarioRunner.h
 * @brief   Simple class to test navigation scenarios
 * @author  Frank Dellaert
 */

#pragma once
#include <gtsam/navigation/ImuFactor.h>
#include <gtsam/navigation/Scenario.h>

namespace gtsam {

class Sampler;

/*
 *  Simple class to test navigation scenarios.
 *  Takes a trajectory scenario as input, and can generate IMU measurements
 */
class ScenarioRunner {
 public:
  ScenarioRunner(const Scenario* scenario, double imuSampleTime = 1.0 / 100.0,
                 double gyroSigma = 0.17, double accSigma = 0.01)
      : scenario_(scenario),
        imuSampleTime_(imuSampleTime),
        gyroNoiseModel_(noiseModel::Isotropic::Sigma(3, gyroSigma)),
        accNoiseModel_(noiseModel::Isotropic::Sigma(3, accSigma)) {}

  // NOTE(frank): hardcoded for now with Z up (gravity points in negative Z)
  // also, uses g=10 for easy debugging
  static Vector3 gravity_n() { return Vector3(0, 0, -10.0); }

  // A gyro simly measures angular velocity in body frame
  Vector3 measured_angular_velocity(double t) const {
    return scenario_->omega_b(t);
  }

  // An accelerometer measures acceleration in body, but not gravity
  Vector3 measured_acceleration(double t) const {
    Rot3 bRn = scenario_->rotation(t).transpose();
    return scenario_->acceleration_b(t) - bRn * gravity_n();
  }

  const double& imuSampleTime() const { return imuSampleTime_; }

  const noiseModel::Diagonal::shared_ptr& gyroNoiseModel() const {
    return gyroNoiseModel_;
  }

  const noiseModel::Diagonal::shared_ptr& accNoiseModel() const {
    return accNoiseModel_;
  }

  Matrix3 gyroCovariance() const { return gyroNoiseModel_->covariance(); }
  Matrix3 accCovariance() const { return accNoiseModel_->covariance(); }

  /// Integrate measurements for T seconds into a PIM
  ImuFactor::PreintegratedMeasurements integrate(double T,
                                                 Sampler* gyroSampler = 0,
                                                 Sampler* accSampler = 0) const;

  /// Predict predict given a PIM
  PoseVelocityBias predict(
      const ImuFactor::PreintegratedMeasurements& pim) const;

  /// Return pose covariance by re-arranging pim.preintMeasCov() appropriately
  Matrix6 poseCovariance(
      const ImuFactor::PreintegratedMeasurements& pim) const {
    Matrix9 cov = pim.preintMeasCov();
    Matrix6 poseCov;
    poseCov << cov.block<3, 3>(6, 6), cov.block<3, 3>(6, 0),  //
        cov.block<3, 3>(0, 6), cov.block<3, 3>(0, 0);
    return poseCov;
  }

  /// Compute a Monte Carlo estimate of the PIM pose covariance using N samples
  Matrix6 estimatePoseCovariance(double T, size_t N = 1000) const;

 private:
  const Scenario* scenario_;
  double imuSampleTime_;
  noiseModel::Diagonal::shared_ptr gyroNoiseModel_, accNoiseModel_;
};

}  // namespace gtsam