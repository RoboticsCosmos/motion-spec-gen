#include <array>
#include <controllers/pid_controller.hpp>
#include <filesystem>
#include <iostream>
#include <kinova_mediator/mediator.hpp>
#include <motion_spec_utils/math_utils.hpp>
#include <motion_spec_utils/solver_utils.hpp>
#include <motion_spec_utils/utils.hpp>
#include <string>

#include "kelo_motion_control/mediator.h"

int main()
{
  // get current file path
  std::filesystem::path path = __FILE__;

  // get the robot urdf path
  std::string robot_urdf =
      (path.parent_path().parent_path() / "urdf" / "freddy.urdf").string();

  // set the base and tool links
  std::string base_link1 = "base_link";
  std::string tool_link_1 = "kinova_left_bracelet_link";
  std::string tool_link_2 = "kinova_right_bracelet_link";

  // initialize the chain
  KDL::Chain kinova_left_chain;
  initialize_robot_chain(robot_urdf, base_link1, tool_link_1, kinova_left_chain);

  KDL::Chain kinova_right_chain;
  initialize_robot_chain(robot_urdf, base_link1, tool_link_2, kinova_right_chain);

  // Initialize the robot structs
  Manipulator kinova_right_state;
  MobileBase freddy_base_state;
  Manipulator kinova_left_state;

  initialize_manipulator_state(kinova_right_chain.getNrOfJoints(),
                               kinova_right_chain.getNrOfSegments(), &kinova_right_state);
  initialize_mobile_base_state(&freddy_base_state);
  initialize_manipulator_state(kinova_left_chain.getNrOfJoints(),
                               kinova_left_chain.getNrOfSegments(), &kinova_left_state);

  // Initialize the robot connections
  // Initialize the Manipulator connections
  kinova_mediator *kinova_right_mediator = new kinova_mediator();
  kinova_right_mediator->initialize(0, 0, 0.0);
  kinova_right_mediator->set_control_mode(2);
  // Initialize the MobileBase connections
  KeloBaseConfig freddy_base_config;
  EthercatConfig freddy_base_ethercat_config;
  initialize_kelo_base(&freddy_base_config, &freddy_base_ethercat_config);
  int result = 0;
  establish_kelo_base_connection(&freddy_base_ethercat_config, "eno1", &result);
  if (result != 0)
  {
    printf("Failed to establish connection to KeloBase\n");
    exit(1);
  }
  // Initialize the Manipulator connections
  kinova_mediator *kinova_left_mediator = new kinova_mediator();
  kinova_left_mediator->initialize(0, 0, 0.0);
  kinova_left_mediator->set_control_mode(2);

  // initialize variables
  double achd_solver_kinova_left_output_torques[7]{};
  double kinova_left_bracelet_table_contact_force_lin_z_vector_z[6] = {0, 0, 1};
  double kinova_right_bracelet_base_lin_vel_x_pid_controller_error_sum;
  double kinova_left_bracelet_base_lin_vel_y_pid_controller_time_step = 1;
  double kinova_right_bracelet_base_lin_vel_y_pid_controller_signal;
  double *achd_solver_kinova_left_alpha[2] = {
      new double[6]{1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      new double[6]{0.0, 1.0, 0.0, 0.0, 0.0, 0.0}};
  double kinova_right_bracelet_table_contact_force_pid_controller_error_sum;
  int achd_solver_fext_kinova_right_nj = 7;
  double kinova_right_bracelet_base_distance_impedance_controller_signal;
  double kinova_left_bracelet_table_contact_force_pid_controller_kp = 20.0;
  double kinova_right_bracelet_base_vel_vector_lin_x[6] = {1, 0, 0, 0, 0, 0};
  double kinova_left_bracelet_table_contact_force_pid_controller_ki = 0.9;
  double fd_solver_robile_root_acceleration[6] = {0.0, 0.0, 9.81, 0.0, 0.0, 0.0};
  double arm_bracelet_link_lin_xy_vel_reference_value = -10.0;
  std::string base_link = "base_link";
  double kinova_left_bracelet_table_contact_force_pid_controller_kd = 0.0;
  double kinova_left_bracelet_base_lin_vel_x_pid_controller_error_sum;
  double kinova_right_bracelet_base_distance;
  double achd_solver_fext_kinova_left_output_external_wrench[6]{};
  int achd_solver_fext_kinova_right_ns = 8;
  double kinova_right_bracelet_base_lin_vel_x_pid_controller_time_step = 1;
  double kinova_right_bracelet_table_contact_force_pid_controller_signal;
  double kinova_right_bracelet_base_lin_vel_x_pid_controller_kd = 0.0;
  double achd_solver_kinova_right_predicted_accelerations[7]{};
  double kinova_right_bracelet_base_lin_vel_y_pid_controller_time_step = 1;
  double kinova_right_bracelet_base_lin_vel_x_pid_controller_ki = 0.9;
  double kinova_left_bracelet_base_lin_vel_x_pid_controller_time_step = 1;
  double kinova_left_bracelet_table_contact_force_embed_map_vector[6] = {0.0, 0.0, 1.0,
                                                                         0.0, 0.0, 0.0};
  double kinova_left_bracelet_base_lin_vel_y_pid_controller_error_sum;
  double kinova_right_bracelet_base_lin_vel_x_pid_controller_kp = 20.0;
  double kinova_left_bracelet_base_lin_vel_y_pid_controller_kp = 20.0;
  double kinova_left_bracelet_base_distance;
  double kinova_left_bracelet_base_distance_impedance_controller_signal;
  double kinova_left_bracelet_table_contact_force_pid_controller_time_step = 1;
  int achd_solver_fext_kinova_left_nj = 7;
  double achd_solver_fext_kinova_right_root_acceleration[6] = {0.0, 0.0, 9.81,
                                                               0.0, 0.0, 0.0};
  double kinova_left_bracelet_base_lin_vel_y_pid_controller_signal;
  double kinova_left_bracelet_base_lin_vel_y_embed_map_vector[6] = {0.0, 1.0, 0.0,
                                                                    0.0, 0.0, 0.0};
  double kinova_right_bracelet_base_lin_vel_x_pid_controller_prev_error;
  double achd_solver_kinova_left_predicted_accelerations[7]{};
  double kinova_right_bracelet_base_lin_vel_x_embed_map_vector[6] = {1.0, 0.0, 0.0,
                                                                     0.0, 0.0, 0.0};
  double kinova_right_bracelet_base_lin_vel_y_pid_controller_kp = 20.0;
  double kinova_right_bracelet_base_lin_vel_y_pid_controller_ki = 0.9;
  double kinova_right_bracelet_table_contact_force_lin_z;
  double achd_solver_kinova_left_feed_forward_torques[7]{};
  double kinova_right_bracelet_base_lin_vel_y_pid_controller_prev_error;
  double kinova_right_bracelet_table_contact_force_lin_z_vector_z[6] = {0, 0, 1};
  double kinova_left_bracelet_table_contact_force_pid_controller_error_sum;
  std::string table = "table";
  double kinova_left_bracelet_base_lin_vel_y_pid_controller_prev_error;
  double kinova_right_bracelet_base_lin_vel_y_pid_controller_kd = 0.0;
  std::string kinova_right_base_link = "kinova_right_base_link";
  double kinova_left_bracelet_base_distance_embed_map_vector[3] = {1.0, 1.0, 1.0};
  double achd_solver_kinova_right_root_acceleration[6] = {0.0, 0.0, 9.81, 0.0, 0.0, 0.0};
  double kinova_left_bracelet_base_distance_impedance_controller_stiffness_diag_mat[1] = {
      1.0};
  double kinova_left_bracelet_base_lin_vel_x_pid_controller_prev_error;
  double kinova_right_bracelet_base_distance_embed_map_vector[3] = {1.0, 1.0, 1.0};
  std::string kinova_left_bracelet_link = "kinova_left_bracelet_link";
  double achd_solver_fext_kinova_right_output_torques[7]{};
  double kinova_left_bracelet_base_lin_vel_y_pid_controller_ki = 0.9;
  int achd_solver_fext_kinova_left_ns = 8;
  double kinova_left_bracelet_base_lin_vel_y_pid_controller_kd = 0.0;
  double kinova_left_bracelet_base_lin_vel_x_twist_embed_map_vector[6] = {1.0, 0.0, 0.0,
                                                                          0.0, 0.0, 0.0};
  int achd_solver_kinova_right_ns = 8;
  double kinova_right_bracelet_table_contact_force_pid_controller_kp = 20.0;
  int achd_solver_kinova_right_nj = 7;
  double kinova_right_bracelet_table_contact_force_pid_controller_ki = 0.9;
  double kinova_right_bracelet_table_contact_force_pid_controller_kd = 0.0;
  int achd_solver_kinova_right_nc = 6;
  double arms_distance_reference_value = 0.5;
  double kinova_right_bracelet_table_contact_force_embed_map_vector[6] = {0.0, 0.0, 1.0,
                                                                          0.0, 0.0, 0.0};
  double achd_solver_kinova_right_output_torques[7]{};
  int achd_solver_kinova_left_nc = 6;
  std::string kinova_left_base_link = "kinova_left_base_link";
  double achd_solver_kinova_right_output_acceleration_energy[6]{};
  int achd_solver_kinova_left_nj = 7;
  std::string kinova_right_bracelet_link = "kinova_right_bracelet_link";
  double kinova_right_bracelet_base_lin_vel_y_pid_controller_error_sum;
  double kinova_right_bracelet_table_contact_force_pid_controller_prev_error;
  int achd_solver_kinova_left_ns = 8;
  double achd_solver_fext_kinova_right_output_external_wrench[6]{};
  double kinova_left_bracelet_table_contact_force_pid_controller_signal;
  double kinova_left_bracelet_table_contact_force_lin_z;
  double kinova_right_bracelet_base_lin_vel_y_twist_embed_map_vector[6] = {0.0, 1.0, 0.0,
                                                                           0.0, 0.0, 0.0};
  double arm_table_contact_force_reference_value = -10.0;
  double *achd_solver_kinova_right_alpha[2] = {
      new double[6]{1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      new double[6]{0.0, 1.0, 0.0, 0.0, 0.0, 0.0}};
  double fd_solver_robile_output_external_wrench[3]{};
  double kinova_left_bracelet_base_lin_vel_x_pid_controller_kp = 20.0;
  double achd_solver_kinova_left_root_acceleration[6] = {0.0, 0.0, 9.81, 0.0, 0.0, 0.0};
  double kinova_left_bracelet_base_lin_vel_x_pid_controller_signal;
  double kinova_right_bracelet_base_distance_impedance_controller_stiffness_diag_mat[1] =
      {1.0};
  double kinova_left_bracelet_base_lin_vel_x_pid_controller_ki = 0.9;
  double kinova_left_bracelet_table_contact_force_pid_controller_prev_error;
  double kinova_right_bracelet_base_vel_vector_lin_y[6] = {0, 1, 0, 0, 0, 0};
  double achd_solver_fext_kinova_left_root_acceleration[6] = {0.0, 0.0, 9.81,
                                                              0.0, 0.0, 0.0};
  double achd_solver_kinova_left_output_acceleration_energy[6]{};
  double kinova_left_bracelet_base_lin_vel_x_pid_controller_kd = 0.0;
  double of_vel_qname_lin_x;
  double of_vel_qname_lin_y;
  double achd_solver_fext_kinova_left_output_torques[7]{};
  double achd_solver_kinova_right_feed_forward_torques[7]{};
  double fd_solver_robile_output_torques[8]{};
  double kinova_right_bracelet_base_lin_vel_x_pid_controller_signal;
  double kinova_left_bracelet_base_vel_vector_lin_x[6] = {1, 0, 0, 0, 0, 0};
  double kinova_right_bracelet_table_contact_force_pid_controller_time_step = 1;
  double kinova_left_bracelet_base_vel_vector_lin_y[6] = {0, 1, 0, 0, 0, 0};

  while (true)
  {
    // Get the robot structs with the data from robots
    get_manipulator_data(&kinova_right_state, kinova_right_mediator);
    get_kelo_base_state(&freddy_base_config, &freddy_base_ethercat_config,
                        freddy_base_state.pivot_angles);
    get_manipulator_data(&kinova_left_state, kinova_left_mediator);

    // controllers
    // pid controller
    computeForce(kinova_left_bracelet_link, table, kinova_left_bracelet_link,
                 kinova_left_bracelet_table_contact_force_lin_z_vector_z,
                 &kinova_left_state, &kinova_left_chain,
                 kinova_left_bracelet_table_contact_force_lin_z);
    double kinova_left_bracelet_table_contact_force_pid_controller_error = 0;
    computeEqualityError(kinova_left_bracelet_table_contact_force_lin_z,
                         arm_table_contact_force_reference_value,
                         kinova_left_bracelet_table_contact_force_pid_controller_error);
    pidController(kinova_left_bracelet_table_contact_force_pid_controller_error,
                  kinova_left_bracelet_table_contact_force_pid_controller_kp,
                  kinova_left_bracelet_table_contact_force_pid_controller_ki,
                  kinova_left_bracelet_table_contact_force_pid_controller_kd,
                  kinova_left_bracelet_table_contact_force_pid_controller_time_step,
                  kinova_left_bracelet_table_contact_force_pid_controller_error_sum,
                  kinova_left_bracelet_table_contact_force_pid_controller_prev_error,
                  kinova_left_bracelet_table_contact_force_pid_controller_signal);

    // pid controller
    computeForwardVelocityKinematics(kinova_left_bracelet_link, base_link, base_link,
                                     kinova_left_bracelet_base_vel_vector_lin_y,
                                     &kinova_left_state, &kinova_left_chain,
                                     of_vel_qname_lin_y);
    double kinova_left_bracelet_base_lin_vel_y_pid_controller_error = 0;
    computeEqualityError(of_vel_qname_lin_y, arm_bracelet_link_lin_xy_vel_reference_value,
                         kinova_left_bracelet_base_lin_vel_y_pid_controller_error);
    pidController(kinova_left_bracelet_base_lin_vel_y_pid_controller_error,
                  kinova_left_bracelet_base_lin_vel_y_pid_controller_kp,
                  kinova_left_bracelet_base_lin_vel_y_pid_controller_ki,
                  kinova_left_bracelet_base_lin_vel_y_pid_controller_kd,
                  kinova_left_bracelet_base_lin_vel_y_pid_controller_time_step,
                  kinova_left_bracelet_base_lin_vel_y_pid_controller_error_sum,
                  kinova_left_bracelet_base_lin_vel_y_pid_controller_prev_error,
                  kinova_left_bracelet_base_lin_vel_y_pid_controller_signal);

    // pid controller
    computeForce(kinova_right_bracelet_link, table, kinova_right_bracelet_link,
                 kinova_right_bracelet_table_contact_force_lin_z_vector_z,
                 &kinova_right_state, &kinova_right_chain,
                 kinova_right_bracelet_table_contact_force_lin_z);
    double kinova_right_bracelet_table_contact_force_pid_controller_error = 0;
    computeEqualityError(kinova_right_bracelet_table_contact_force_lin_z,
                         arm_table_contact_force_reference_value,
                         kinova_right_bracelet_table_contact_force_pid_controller_error);
    pidController(kinova_right_bracelet_table_contact_force_pid_controller_error,
                  kinova_right_bracelet_table_contact_force_pid_controller_kp,
                  kinova_right_bracelet_table_contact_force_pid_controller_ki,
                  kinova_right_bracelet_table_contact_force_pid_controller_kd,
                  kinova_right_bracelet_table_contact_force_pid_controller_time_step,
                  kinova_right_bracelet_table_contact_force_pid_controller_error_sum,
                  kinova_right_bracelet_table_contact_force_pid_controller_prev_error,
                  kinova_right_bracelet_table_contact_force_pid_controller_signal);

    // pid controller
    computeForwardVelocityKinematics(kinova_right_bracelet_link, base_link, base_link,
                                     kinova_right_bracelet_base_vel_vector_lin_x,
                                     &kinova_right_state, &kinova_right_chain,
                                     of_vel_qname_lin_x);
    double kinova_right_bracelet_base_lin_vel_x_pid_controller_error = 0;
    computeEqualityError(of_vel_qname_lin_x, arm_bracelet_link_lin_xy_vel_reference_value,
                         kinova_right_bracelet_base_lin_vel_x_pid_controller_error);
    pidController(kinova_right_bracelet_base_lin_vel_x_pid_controller_error,
                  kinova_right_bracelet_base_lin_vel_x_pid_controller_kp,
                  kinova_right_bracelet_base_lin_vel_x_pid_controller_ki,
                  kinova_right_bracelet_base_lin_vel_x_pid_controller_kd,
                  kinova_right_bracelet_base_lin_vel_x_pid_controller_time_step,
                  kinova_right_bracelet_base_lin_vel_x_pid_controller_error_sum,
                  kinova_right_bracelet_base_lin_vel_x_pid_controller_prev_error,
                  kinova_right_bracelet_base_lin_vel_x_pid_controller_signal);

    // pid controller
    computeForwardVelocityKinematics(kinova_right_bracelet_link, base_link, base_link,
                                     kinova_right_bracelet_base_vel_vector_lin_y,
                                     &kinova_right_state, &kinova_right_chain,
                                     of_vel_qname_lin_y);
    double kinova_right_bracelet_base_lin_vel_y_pid_controller_error = 0;
    computeEqualityError(of_vel_qname_lin_y, arm_bracelet_link_lin_xy_vel_reference_value,
                         kinova_right_bracelet_base_lin_vel_y_pid_controller_error);
    pidController(kinova_right_bracelet_base_lin_vel_y_pid_controller_error,
                  kinova_right_bracelet_base_lin_vel_y_pid_controller_kp,
                  kinova_right_bracelet_base_lin_vel_y_pid_controller_ki,
                  kinova_right_bracelet_base_lin_vel_y_pid_controller_kd,
                  kinova_right_bracelet_base_lin_vel_y_pid_controller_time_step,
                  kinova_right_bracelet_base_lin_vel_y_pid_controller_error_sum,
                  kinova_right_bracelet_base_lin_vel_y_pid_controller_prev_error,
                  kinova_right_bracelet_base_lin_vel_y_pid_controller_signal);

    // impedance controller
    double kinova_right_bracelet_base_distance_impedance_controller_stiffness_error = 0;
    computeDistance(new std::string[2]{kinova_right_bracelet_link, kinova_right_base_link},
                    kinova_right_bracelet_link, &kinova_right_state, &kinova_right_chain,
                    kinova_right_bracelet_base_distance);
    computeEqualityError(
        kinova_right_bracelet_base_distance, arms_distance_reference_value,
        kinova_right_bracelet_base_distance_impedance_controller_stiffness_error);
    impedanceController(
        kinova_right_bracelet_base_distance_impedance_controller_stiffness_error, 0.0,
        kinova_right_bracelet_base_distance_impedance_controller_stiffness_diag_mat,
        new double[1]{0.0},
        kinova_right_bracelet_base_distance_impedance_controller_signal);

    // impedance controller
    double kinova_left_bracelet_base_distance_impedance_controller_stiffness_error = 0;
    computeDistance(new std::string[2]{kinova_left_bracelet_link, kinova_left_base_link},
                    kinova_left_bracelet_link, &kinova_left_state, &kinova_left_chain,
                    kinova_left_bracelet_base_distance);
    computeEqualityError(
        kinova_left_bracelet_base_distance, arms_distance_reference_value,
        kinova_left_bracelet_base_distance_impedance_controller_stiffness_error);
    impedanceController(
        kinova_left_bracelet_base_distance_impedance_controller_stiffness_error, 0.0,
        kinova_left_bracelet_base_distance_impedance_controller_stiffness_diag_mat,
        new double[1]{0.0},
        kinova_left_bracelet_base_distance_impedance_controller_signal);

    // pid controller
    computeForwardVelocityKinematics(kinova_left_bracelet_link, base_link, base_link,
                                     kinova_left_bracelet_base_vel_vector_lin_x,
                                     &kinova_left_state, &kinova_left_chain,
                                     of_vel_qname_lin_x);
    double kinova_left_bracelet_base_lin_vel_x_pid_controller_error = 0;
    computeEqualityError(of_vel_qname_lin_x, arm_bracelet_link_lin_xy_vel_reference_value,
                         kinova_left_bracelet_base_lin_vel_x_pid_controller_error);
    pidController(kinova_left_bracelet_base_lin_vel_x_pid_controller_error,
                  kinova_left_bracelet_base_lin_vel_x_pid_controller_kp,
                  kinova_left_bracelet_base_lin_vel_x_pid_controller_ki,
                  kinova_left_bracelet_base_lin_vel_x_pid_controller_kd,
                  kinova_left_bracelet_base_lin_vel_x_pid_controller_time_step,
                  kinova_left_bracelet_base_lin_vel_x_pid_controller_error_sum,
                  kinova_left_bracelet_base_lin_vel_x_pid_controller_prev_error,
                  kinova_left_bracelet_base_lin_vel_x_pid_controller_signal);

    // embed maps
    for (size_t i = 0;
         i < sizeof(kinova_right_bracelet_base_lin_vel_x_embed_map_vector) /
                 sizeof(kinova_right_bracelet_base_lin_vel_x_embed_map_vector[0]);
         i++)
    {
      if (kinova_right_bracelet_base_lin_vel_x_embed_map_vector[i] != 0.0)
      {
        achd_solver_kinova_right_output_acceleration_energy[i] +=
            kinova_right_bracelet_base_lin_vel_x_pid_controller_signal;
      }
    }
    for (size_t i = 0;
         i < sizeof(kinova_right_bracelet_base_lin_vel_y_twist_embed_map_vector) /
                 sizeof(kinova_right_bracelet_base_lin_vel_y_twist_embed_map_vector[0]);
         i++)
    {
      if (kinova_right_bracelet_base_lin_vel_y_twist_embed_map_vector[i] != 0.0)
      {
        achd_solver_kinova_right_output_acceleration_energy[i] +=
            kinova_right_bracelet_base_lin_vel_y_pid_controller_signal;
      }
    }
    for (size_t i = 0;
         i < sizeof(kinova_left_bracelet_table_contact_force_embed_map_vector) /
                 sizeof(kinova_left_bracelet_table_contact_force_embed_map_vector[0]);
         i++)
    {
      if (kinova_left_bracelet_table_contact_force_embed_map_vector[i] != 0.0)
      {
        achd_solver_fext_kinova_left_output_external_wrench[i] +=
            kinova_left_bracelet_table_contact_force_pid_controller_signal;
      }
    }
    double fd_solver_robile_output_external_wrench_vector[3]{};
    findVector(kinova_left_bracelet_link, kinova_left_base_link, &kinova_left_state,
               &kinova_left_chain, fd_solver_robile_output_external_wrench_vector);
    double fd_solver_robile_output_external_wrench_norm_vector[3]{};
    findVectorNorm(fd_solver_robile_output_external_wrench_vector,
                   fd_solver_robile_output_external_wrench_norm_vector);
    for (size_t i = 0;
         i < sizeof(fd_solver_robile_output_external_wrench_norm_vector) /
                 sizeof(fd_solver_robile_output_external_wrench_norm_vector[0]);
         i++)
    {
      if (fd_solver_robile_output_external_wrench_norm_vector[i] != 0.0)
      {
        fd_solver_robile_output_external_wrench[i] +=
            kinova_left_bracelet_base_distance_impedance_controller_signal *
            fd_solver_robile_output_external_wrench_norm_vector[i];
      }
    }
    double fd_solver_robile_output_external_wrench_vector[3]{};
    findVector(kinova_right_bracelet_link, kinova_right_base_link, &kinova_right_state,
               &kinova_right_chain, fd_solver_robile_output_external_wrench_vector);
    double fd_solver_robile_output_external_wrench_norm_vector[3]{};
    findVectorNorm(fd_solver_robile_output_external_wrench_vector,
                   fd_solver_robile_output_external_wrench_norm_vector);
    for (size_t i = 0;
         i < sizeof(fd_solver_robile_output_external_wrench_norm_vector) /
                 sizeof(fd_solver_robile_output_external_wrench_norm_vector[0]);
         i++)
    {
      if (fd_solver_robile_output_external_wrench_norm_vector[i] != 0.0)
      {
        fd_solver_robile_output_external_wrench[i] +=
            kinova_right_bracelet_base_distance_impedance_controller_signal *
            fd_solver_robile_output_external_wrench_norm_vector[i];
      }
    }
    for (size_t i = 0;
         i < sizeof(kinova_left_bracelet_base_lin_vel_x_twist_embed_map_vector) /
                 sizeof(kinova_left_bracelet_base_lin_vel_x_twist_embed_map_vector[0]);
         i++)
    {
      if (kinova_left_bracelet_base_lin_vel_x_twist_embed_map_vector[i] != 0.0)
      {
        achd_solver_kinova_left_output_acceleration_energy[i] +=
            kinova_left_bracelet_base_lin_vel_x_pid_controller_signal;
      }
    }
    for (size_t i = 0;
         i < sizeof(kinova_left_bracelet_base_lin_vel_y_embed_map_vector) /
                 sizeof(kinova_left_bracelet_base_lin_vel_y_embed_map_vector[0]);
         i++)
    {
      if (kinova_left_bracelet_base_lin_vel_y_embed_map_vector[i] != 0.0)
      {
        achd_solver_kinova_left_output_acceleration_energy[i] +=
            kinova_left_bracelet_base_lin_vel_y_pid_controller_signal;
      }
    }
    for (size_t i = 0;
         i < sizeof(kinova_right_bracelet_table_contact_force_embed_map_vector) /
                 sizeof(kinova_right_bracelet_table_contact_force_embed_map_vector[0]);
         i++)
    {
      if (kinova_right_bracelet_table_contact_force_embed_map_vector[i] != 0.0)
      {
        achd_solver_fext_kinova_right_output_external_wrench[i] +=
            kinova_right_bracelet_table_contact_force_pid_controller_signal;
      }
    }

    // solvers
    // achd_solver
    double achd_solver_kinova_right_beta[6]{};
    for (size_t i = 0; i < 6; i++)
    {
      achd_solver_kinova_right_beta[i] = achd_solver_kinova_right_root_acceleration[i];
    }
    double *achd_solver_kinova_right_ext_wrench[kinova_right_state.ns];
    for (size_t i = 0; i < 7; i++)
    {
      achd_solver_kinova_right_ext_wrench[i] =
          new double[6]{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    }
    add(achd_solver_kinova_right_output_acceleration_energy,
        achd_solver_kinova_right_beta, achd_solver_kinova_right_beta, 6);
    achd_solver(&kinova_right_state, &kinova_right_chain, achd_solver_kinova_right_nc,
                achd_solver_kinova_right_root_acceleration,
                achd_solver_kinova_right_alpha, achd_solver_kinova_right_beta,
                achd_solver_kinova_right_ext_wrench,
                achd_solver_kinova_right_feed_forward_torques,
                achd_solver_kinova_right_predicted_accelerations,
                achd_solver_kinova_right_output_torques);

    // achd_solver_fext
    double *achd_solver_fext_kinova_left_ext_wrench[kinova_left_state.ns];
    for (size_t i = 0; i < kinova_left_state.ns - 1; i++)
    {
      achd_solver_fext_kinova_left_ext_wrench[i] =
          new double[6]{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    }
    double achd_solver_fext_kinova_left_ext_wrench_tool[6]{};
    add(achd_solver_fext_kinova_left_output_external_wrench,
        achd_solver_fext_kinova_left_ext_wrench_tool,
        achd_solver_fext_kinova_left_ext_wrench_tool, 6);
    achd_solver_fext_kinova_left_ext_wrench[kinova_left_state.ns - 1] =
        achd_solver_fext_kinova_left_ext_wrench_tool;
    achd_solver_fext(&kinova_left_state, &kinova_left_chain,
                     achd_solver_fext_kinova_left_ext_wrench,
                     achd_solver_fext_kinova_left_output_torques);

    // base_fd_solver
    double fd_solver_robile_platform_force[3]{};
    add(fd_solver_robile_output_external_wrench, fd_solver_robile_platform_force,
        fd_solver_robile_platform_force, 3);
    add(fd_solver_robile_output_external_wrench, fd_solver_robile_platform_force,
        fd_solver_robile_platform_force, 3);
    base_fd_solver(&freddy_base_state, fd_solver_robile_platform_force,
                   fd_solver_robile_output_torques);

    // achd_solver
    double achd_solver_kinova_left_beta[6]{};
    for (size_t i = 0; i < 6; i++)
    {
      achd_solver_kinova_left_beta[i] = achd_solver_kinova_left_root_acceleration[i];
    }
    double *achd_solver_kinova_left_ext_wrench[kinova_left_state.ns];
    for (size_t i = 0; i < 7; i++)
    {
      achd_solver_kinova_left_ext_wrench[i] = new double[6]{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    }
    add(achd_solver_kinova_left_output_acceleration_energy, achd_solver_kinova_left_beta,
        achd_solver_kinova_left_beta, 6);
    achd_solver(&kinova_left_state, &kinova_left_chain, achd_solver_kinova_left_nc,
                achd_solver_kinova_left_root_acceleration, achd_solver_kinova_left_alpha,
                achd_solver_kinova_left_beta, achd_solver_kinova_left_ext_wrench,
                achd_solver_kinova_left_feed_forward_torques,
                achd_solver_kinova_left_predicted_accelerations,
                achd_solver_kinova_left_output_torques);

    // achd_solver_fext
    double *achd_solver_fext_kinova_right_ext_wrench[kinova_right_state.ns];
    for (size_t i = 0; i < kinova_right_state.ns - 1; i++)
    {
      achd_solver_fext_kinova_right_ext_wrench[i] =
          new double[6]{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    }
    double achd_solver_fext_kinova_right_ext_wrench_tool[6]{};
    add(achd_solver_fext_kinova_right_output_external_wrench,
        achd_solver_fext_kinova_right_ext_wrench_tool,
        achd_solver_fext_kinova_right_ext_wrench_tool, 6);
    achd_solver_fext_kinova_right_ext_wrench[kinova_right_state.ns - 1] =
        achd_solver_fext_kinova_right_ext_wrench_tool;
    achd_solver_fext(&kinova_right_state, &kinova_right_chain,
                     achd_solver_fext_kinova_right_ext_wrench,
                     achd_solver_fext_kinova_right_output_torques);

    // Command the torques to the robots
    double kinova_right_cmd_tau[7]{};
    add(achd_solver_kinova_right_output_torques, kinova_right_cmd_tau,
        kinova_right_cmd_tau, 7);
    add(achd_solver_fext_kinova_right_output_torques, kinova_right_cmd_tau,
        kinova_right_cmd_tau, 7);
    set_manipulator_torques(&kinova_right_state, kinova_right_mediator,
                            kinova_right_cmd_tau);
    double freddy_base_cmd_tau[8]{};
    add(fd_solver_robile_output_torques, freddy_base_cmd_tau, freddy_base_cmd_tau, 8);
    set_kelo_base_torques(&freddy_base_config, &freddy_base_ethercat_config,
                          freddy_base_cmd_tau);
    double kinova_left_cmd_tau[7]{};
    add(achd_solver_kinova_left_output_torques, kinova_left_cmd_tau, kinova_left_cmd_tau,
        7);
    add(achd_solver_fext_kinova_left_output_torques, kinova_left_cmd_tau,
        kinova_left_cmd_tau, 7);
    set_manipulator_torques(&kinova_left_state, kinova_left_mediator,
                            kinova_left_cmd_tau);
  }

  return 0;
}