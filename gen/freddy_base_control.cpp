#include "kelo_motion_control/mediator.h"
#include <array>
#include <string>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <controllers/pid_controller.hpp>
#include <motion_spec_utils/utils.hpp>
#include <motion_spec_utils/math_utils.hpp>
#include <motion_spec_utils/solver_utils.hpp>
#include <csignal>

#include <unsupported/Eigen/MatrixFunctions>

volatile sig_atomic_t flag = 0;

void handle_signal(int sig)
{
  static int signal_caught = 0;
  if (!signal_caught)
  {
    signal_caught = 1;
    flag = 1;
    printf("Caught signal %d (%s)\n", sig, strsignal(sig));
  }
}

int main(int argc, char **argv)
{
  // handle signals
  struct sigaction sa;
  sa.sa_handler = handle_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  for (int i = 1; i < NSIG; ++i)
  {
    if (sigaction(i, &sa, NULL) == -1)
    {
      perror("sigaction");
    }
  }

  // read the platform force from the command line
  double pf[3] = {0.0, 0.0, 0.0};
  if (argc == 4)
  {
    pf[0] = std::stod(argv[1]);
    pf[1] = std::stod(argv[2]);
    pf[2] = std::stod(argv[3]);
  }
  else
  {
    printf(
        "Usage: ./freddy_base_align <platform_force_x> <platform_force_y> <platform_torque_z>\n");
    exit(1);
  }

  // Initialize the robot structs
  KeloBaseConfig *kelo_base_config = new KeloBaseConfig();
  kelo_base_config->nWheels = 4;
  int index_to_EtherCAT[4] = {6, 7, 3, 4};
  kelo_base_config->index_to_EtherCAT = index_to_EtherCAT;
  kelo_base_config->radius = 0.115 / 2;
  kelo_base_config->castor_offset = 0.01;
  kelo_base_config->half_wheel_distance = 0.0775 / 2;
  double wheel_coordinates[8] = {0.188, 0.2075, -0.188, 0.2075, -0.188, -0.2075, 0.188, -0.2075};
  kelo_base_config->wheel_coordinates = wheel_coordinates;
  double pivot_angles_deviation[4] = {5.310, 5.533, 1.563, 1.625};
  kelo_base_config->pivot_angles_deviation = pivot_angles_deviation;

  EthercatConfig *ethercat_config = new EthercatConfig();

  MobileBase<Robile> freddy_base;
  Robile robile;
  robile.ethercat_config = ethercat_config;
  robile.kelo_base_config = kelo_base_config;

  freddy_base.mediator = &robile;
  freddy_base.state = new MobileBaseState();

  Freddy robot = {nullptr, nullptr, &freddy_base};

  // get current file path
  std::filesystem::path path = __FILE__;

  // get the robot urdf path
  std::string robot_urdf =
      (path.parent_path().parent_path() / "urdf" / "freddy_corrected_base.urdf").string();

  char ethernet_interface[100] = "eno1";
  initialize_robot(&robot, robot_urdf, ethernet_interface);

  const double desired_frequency = 1000.0;                                             // Hz
  const auto desired_period = std::chrono::duration<double>(1.0 / desired_frequency);  // s
  double control_loop_timestep = desired_period.count();                               // s
  double *control_loop_dt = &control_loop_timestep;                                    // s

  // pid controller variables
  double Kp = 3.0;
  double Ki = 0.5;
  double Kd = 0.05;

  double w1_lin_prev_error = 0.0;
  double w1_lin_error_sum = 0.0;
  double w1_ang_prev_error = 0.0;
  double w1_ang_error_sum = 0.0;

  double w2_lin_prev_error = 0.0;
  double w2_lin_error_sum = 0.0;
  double w2_ang_prev_error = 0.0;
  double w2_ang_error_sum = 0.0;

  double w3_lin_prev_error = 0.0;
  double w3_lin_error_sum = 0.0;
  double w3_ang_prev_error = 0.0;
  double w3_ang_error_sum = 0.0;

  double w4_lin_prev_error = 0.0;
  double w4_lin_error_sum = 0.0;
  double w4_ang_prev_error = 0.0;
  double w4_ang_error_sum = 0.0;

  update_base_state(robot.mobile_base->mediator->kelo_base_config,
                    robot.mobile_base->mediator->ethercat_config);
  get_robot_data(&robot, *control_loop_dt);

  int count = 0;

  while (true)
  {
    auto start_time = std::chrono::high_resolution_clock::now();

    if (flag)
    {
      printf("Exiting somewhat cleanly...\n");
      free_robot_data(&robot);
      exit(0);
    }

    count++;
    // printf("\n");
    printf("count: %d\n", count);

    // update_base_state(robot.mobile_base->mediator->kelo_base_config,
    //                   robot.mobile_base->mediator->ethercat_config);
    get_robot_data(&robot, *control_loop_dt);

    // solver
    double platform_force[3] = {pf[0], pf[1], pf[2]};  // [N], [N], [Nm]

    double lin_offsets[robot.mobile_base->mediator->kelo_base_config->nWheels];
    double ang_offsets[robot.mobile_base->mediator->kelo_base_config->nWheels];
    get_pivot_alignment_offsets(&robot, platform_force, lin_offsets, ang_offsets);

    Eigen::Vector2d lin_pf = Eigen::Vector2d(platform_force[0], platform_force[1]);

    double lin_signal_w1 = 0.0;
    double ang_signal_w1 = 0.0;
    double lin_signal_w2 = 0.0;
    double ang_signal_w2 = 0.0;
    double lin_signal_w3 = 0.0;
    double ang_signal_w3 = 0.0;
    double lin_signal_w4 = 0.0;
    double ang_signal_w4 = 0.0;

    double kp[4] = {Kp, Kp, Kp, Kp};
    double ki[4] = {Ki, Ki, Ki, Ki};
    double kd[4] = {Kd, Kd, Kd, Kd};

    pidController(lin_offsets[0], kp[0], ki[0], kd[0], control_loop_timestep, w1_lin_error_sum,
                  10.0, w1_lin_prev_error, lin_signal_w1);
    pidController(ang_offsets[0], kp[0], ki[0], kd[0], control_loop_timestep, w1_ang_error_sum,
                  10.0, w1_ang_prev_error, ang_signal_w1);

    pidController(lin_offsets[1], kp[1], ki[1], kd[1], control_loop_timestep, w2_lin_error_sum,
                  10.0, w2_lin_prev_error, lin_signal_w2);
    pidController(ang_offsets[1], kp[1], ki[1], kd[1], control_loop_timestep, w2_ang_error_sum,
                  10.0, w2_ang_prev_error, ang_signal_w2);

    pidController(lin_offsets[2], kp[2], ki[2], kd[2], control_loop_timestep, w3_lin_error_sum,
                  10.0, w3_lin_prev_error, lin_signal_w3);
    pidController(ang_offsets[2], kp[2], ki[2], kd[2], control_loop_timestep, w3_ang_error_sum,
                  10.0, w3_ang_prev_error, ang_signal_w3);

    pidController(lin_offsets[3], kp[3], ki[3], kd[3], control_loop_timestep, w4_lin_error_sum,
                  10.0, w4_lin_prev_error, lin_signal_w4);
    pidController(ang_offsets[3], kp[3], ki[3], kd[3], control_loop_timestep, w4_ang_error_sum,
                  10.0, w4_ang_prev_error, ang_signal_w4);

    double lin_signals[4] = {lin_signal_w1, lin_signal_w2, lin_signal_w3, lin_signal_w4};
    double ang_signals[4] = {ang_signal_w1, ang_signal_w2, ang_signal_w3, ang_signal_w4};

    printf("pivot angles: ");
    print_array(robot.mobile_base->state->pivot_angles, 4);

    // base solver
    double tau_wheel_c[8]{};
    base_fd_solver_with_alignment(&robot, platform_force, lin_signals, ang_signals, tau_wheel_c);
    printf("torques1: ");
    print_array(tau_wheel_c, 8);

    // set torques
    double tau_limit = 5.0;
    for (size_t i = 0; i < 8; i++)
    {
      if (tau_wheel_c[i] > tau_limit)
      {
        tau_wheel_c[i] = tau_limit;
      }
      else if (tau_wheel_c[i] < -tau_limit)
      {
        tau_wheel_c[i] = -tau_limit;
      }
    }

    // printf("torques3: ");
    // print_array(tau_wheel_c, 8);

    printf("\n");

    if (count > 2)
    {
      // raise(SIGINT);
      set_mobile_base_torques(&robot, tau_wheel_c);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration<double>(end_time - start_time);

    // if the elapsed time is less than the desired period, busy wait
    while (elapsed_time < desired_period)
    {
      end_time = std::chrono::high_resolution_clock::now();
      elapsed_time = std::chrono::duration<double>(end_time - start_time);
    }
    control_loop_timestep = elapsed_time.count();
    // std::cout << "control loop timestep: " << control_loop_timestep << std::endl;
  }

  free_robot_data(&robot);

  return 0;
}