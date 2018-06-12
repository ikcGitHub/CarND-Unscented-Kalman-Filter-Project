#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 * This is scaffolding, do not modify
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 30;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 30;
  
  //DO NOT MODIFY measurement noise values below these are provided by the sensor manufacturer.
  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;
  //DO NOT MODIFY measurement noise values above these are provided by the sensor manufacturer.
  
  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.

  Hint: one or more values initialized above might be wildly off...
  */

  ///* initially set to false, set to true in first call of ProcessMeasurement
  is_initialized_ = false;

  // Set state dimension
  int n_x_ = 5;

  // Set augmented dimension
  int n_aug_ = 7;

  // Define spreading parameter
  double lambda_ = 3 - n_aug_;

  // Create state vector
  VectorXd x_ = VectorXd(n_x_);
  x_ << 0,
	    0,
	    0,
	    0,
	    0;

  // Create covariance matrix
  MatrixXd P_ = MatrixXd(n_x_, n_x_);
  P_ << 1, 0, 0, 0, 0,
	    0, 1, 0, 0, 0,
	    0, 0, 1, 0, 0,
	    0, 0, 0, 1, 0,
	    0, 0, 0, 0, 1;

  // Create predicted sigma points matrix
  Xsig_pred_ = MatrixXd(n_x_, 2 * n_aug_ + 1);

  ///* time when the state is true, in us
  time_us_ = 0.0;

  // Create vector for weights
  VectorXd weights_ = VectorXd(2 * n_aug_ + 1);
  // Set weights
  // Initialize the first element
  weights_(0) = lambda_ / (lambda_ + n_aug_);
  // Initialize the rest of elements
  int i = 0;
  for (i = 1; i < 2 * n_aug_ + 1; i++) {
	  weights_(i) = 0.5 / (n_aug_ + lambda_);
  }

  //// Set measurement dimension, radar can measure r, phi, and r_dot
  //int n_z = 3;

  // Create radar covariance matrix
  MatrixXd R_radar_ = MatrixXd(3, 3);
  R_radar_ << std_radr_ * std_radr_,                         0,                       0,
	                              0, std_radphi_ * std_radphi_,                       0,
	                              0,                         0, std_radrd_ * std_radrd_;

  // Create lidar covariance matrix
  MatrixXd R_lidar_ = MatrixXd(2, 2);
  R_lidar_ << std_laspx_ * std_laspx_,                       0,
	                                0, std_laspy_ * std_laspy_;
}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */

   /*****************************************************************************
   *  Initialization
   ****************************************************************************/
	// TODO : Check if data is initialized ?
  if (!is_initialized_){
    /**
    TODO:
      * Initialize the state ekf_.x_ with the first measurement.
      * Create the covariance matrix.
      * Remember: you'll need to convert radar from polar to cartesian coordinates.
    */
	  if (meas_package.sensor_type == MeasurementPackage::RADAR) {
	     /**
             Convert radar from polar to cartesian coordinates and initialize state.
             */
	     float rho = meas_package.raw_measurements_[0]; // range
	     float phi = meas_package.raw_measurements_[1]; // bearing
	     float rho_dot = meas_package.raw_measurements_[2]; // velocity of rho

	     float px = rho * cos(phi); // position x
	     float py = rho * sin(phi); // position y
	     float vx = rho_dot * cos(phi); // velocity x
	     float vy = rho_dot * sin(phi); // velocity y
	     float v = sqrt(vx * vx + vy * vy); // velocity
		  
	     x_ << px, py, v, 0, 0;
		  
	  }
	  else if (meas_package.sensor_type == MeasurementPackage::LASER){
	    /**
            Initialize state.
            */

	    float px = meas_package.raw_measurements_[0]; // position x
	    float py = meas_package.raw_measurements_[1]; // position y

	    x_ << px, py, 0, 0, 0;
	  }
	  
	  // Initial measurement timestamp
	  time_us_ = meas_package.timestamp_;
	  
          // done initializing, no need to predict or update
          is_initialized_ = true;
          return;
  }
   /*****************************************************************************
   *  Prediction
   ****************************************************************************/
	// TODO : Call predition step with calculated time interval
	
	// Calculate time interval dt
	float dt = (meas_package.timestamp_ - time_us_) / 1000000.0;
	// Update measurement timestamp  
	time_us_ = meas_package.timestamp_;
	// Call prediction
	Prediction(dt);
	
   /*****************************************************************************
   *  Update
   ****************************************************************************/
	// TODO : Call update step with given senser type
	
	if (meas_package.sensor_type == MeasurementPackage::RADAR) {
		UpdateRadar(meas_package);	
	}
	else if (meas_package.sensor_type == MeasurementPackage::LASER) {
		UpdateLidar(meas_package);
	}
}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
  /**
  TODO:

  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */

  ///*****************************************************************************
  //*  Generate Sigma Points
  //****************************************************************************/
  //// TODO: Generate sigma points

  //// Create sigma point matrix
  //MatrixXd Xsig = MatrixXd(n_x_, 2 * n_x_ + 1);

  //// Calculate square root of P
  //MatrixXd A = P_.llt().matrixL();

  //// Set lambda for sigma points
  //lambda_ = 3 - n_x_;

  //// Set sigma points as columns of matrix Xsig
  //Xsig.col(0) = x_;

  //int i = 0;
  //float sig_sqrt = sqrt(lambda_ + n_x_);
  //for (i = 0; i < n_x_; i++) {
	 // Xsig.col(i + 1) = x_ + A.col(i) * sig_sqrt;
	 // Xsig.col(i + 1 + n_x_) = x_ - A.col(i) * sig_sqrt;
  //}

  /*****************************************************************************
  *  Augment Sigma Points
  ****************************************************************************/
  // TODO : Augment Sigma Points

  // Create augmented mean vector
  VectorXd x_aug = VectorXd(n_aug_);

  // Create augmented state covariance
  MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);

  // Create augmented sigma point matrix
  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);

  //// Set lambda for augmented sigma points
  //lambda_ = 3 - n_aug_;

  // Create augmented mean state
  x_aug.head(5) = x_;
  x_aug(5) = 0;
  x_aug(6) = 0;

  // Create augmented covariance matrix
  P_aug.fill(0.0);
  P_aug.topLeftCorner(5, 5) = P_;
  P_aug(5, 5) = std_a_ * std_a_;
  P_aug(6, 6) = std_yawdd_ * std_yawdd_;

  // Create square root matrix
  MatrixXd L = P_aug.llt().matrixL();

  //create augmented sigma points
  Xsig_aug.col(0) = x_aug;

  int i = 0;
  float sig_sqrt = sqrt(lambda_ + n_aug_);
  for (i = 0; i < n_aug_; i++) {
	  Xsig_aug.col(i + 1)          = x_aug + L.col(i) * sig_sqrt;
	  Xsig_aug.col(i + 1 + n_aug_) = x_aug - L.col(i) * sig_sqrt;
  }

  /*****************************************************************************
  *  Predict Sigma Points
  ****************************************************************************/

  // TODO : Predict sigma points

  int i = 0;
  for (i = 0; i < 2 * n_aug_ + 1; i++) {
	  // Read values from current state vector
	  double p_x      = Xsig_aug(0, i);
	  double p_y      = Xsig_aug(1, i);
	  double v        = Xsig_aug(2, i);
	  double yaw      = Xsig_aug(3, i);
	  double yawd     = Xsig_aug(4, i);
	  double nu_a     = Xsig_aug(5, i);
	  double nu_yawdd = Xsig_aug(6, i);

	  // Initialize predicted state
	  double px_p, py_p;

	  // Avoid division by zero
	  if (fabs(yawd) > 0.001) {
		  px_p = p_x + v / yawd * ( sin(yaw + yawd * delta_t) - sin(yaw));
		  py_p = p_y + v / yawd * (-cos(yaw + yawd * delta_t) + cos(yaw));
	  }
	  else {
		  px_p = p_x + v * delta_t * cos(yaw);
		  py_p = p_y + v * delta_t * sin(yaw);
	  }

	  //predict sigma points

	  double v_p = v;
	  double yaw_p = yaw + yawd * delta_t;
	  double yawd_p = yawd;

	  // Add Noise
	  px_p += 0.5 * nu_a * delta_t * delta_t * cos(yaw);
	  py_p += 0.5 * nu_a * delta_t * delta_t * sin(yaw);
	  v_p += nu_a * delta_t;
	  yaw_p += 0.5 * nu_yawdd * delta_t * delta_t;
	  yawd_p += nu_yawdd * delta_t;

	  //write predicted sigma points into right column
	  Xsig_pred(0, i) = px_p;
	  Xsig_pred(1, i) = py_p;
	  Xsig_pred(2, i) = v_p;
	  Xsig_pred(3, i) = yaw_p;
	  Xsig_pred(4, i) = yawd_p;
  }

  /*****************************************************************************
  *  Calculate mean and variance
  ****************************************************************************/
  // TODO : Calculate mean
  // TODO : Calculate variance
  // TODO : Normalize angles

  // Predict state mean
  for (i = 0; i < 2 * n_aug_ + 1; i++) {
	  x_ += weights_(i) * Xsig_pred_.col(i);
  }

  // Predict state covariance matrix
  for (i = 0; i < 2 * n_aug_ + 1; i++) {
	  VectorXd x_diff = Xsig_pred_.col(i) - x_;

	  while (x_diff(3) > M_PI) {
		  x_diff(3) -= 2.0 * M_PI;
	  }

	  while (x_diff(3) < -M_PI) {
		  x_diff(3) += 2.0 * M_PI;
	  }

	  P_ += weights_(i) * x_diff * x_diff.transpose();
  }

}

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */

  // TODO: Extract data from measurement
  // TODO : Generate measurement sigma points
  // TODO : Update predicted measurement mean
  // TODO : Update predicted measurement covariance
  // TODO : Add noise
  // TODO : Create cross covariance matrix
  // TODO : Calculate Kalman gain Kalman
  // TODO : Calculate residual error
  // TODO : Normalize angles
  // TODO : Update state mean vector
  // TODO : Update state covariance matrix
  // TODO : Update NIS Radar
}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */

  // TODO: Extract data from measurement
  // TODO : Generate measurement sigma points
  // TODO : Update predicted measurement mean
  // TODO : Update predicted measurement covariance
  // TODO : Add noise
  // TODO : Create cross covariance matrix
  // TODO : Calculate Kalman gain Kalman
  // TODO : Calculate residual error
  // TODO : Normalize angles
  // TODO : Update state mean vector
  // TODO : Update state covariance matrix
  // TODO : Update NIS Lidar

}
