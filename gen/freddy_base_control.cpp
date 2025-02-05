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

#include "motion_spec_utils/log_structs.hpp"

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

// void ramp(double current_vel, double max_vel,

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
  int index_to_EtherCAT[4] = {3, 4, 6, 7};
  kelo_base_config->index_to_EtherCAT = index_to_EtherCAT;
  kelo_base_config->radius = 0.115 / 2;
  kelo_base_config->castor_offset = 0.01;
  kelo_base_config->half_wheel_distance = 0.0775 / 2;
  double wheel_coordinates[8] = {0.195, 0.21, -0.195, 0.21, -0.195, -0.21, 0.195, -0.21};
  kelo_base_config->wheel_coordinates = wheel_coordinates;
  double pivot_angles_deviation[4] = {0.0, 0.0, 0.0, 0.0};
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
  double Kp = 1.5;
  double Ki = 0.1;
  double Kd = 0.0;

  double w_dist_prev_error[4] = {0.0, 0.0, 0.0, 0.0};
  double w_dist_error_sum[4] = {0.0, 0.0, 0.0, 0.0};

  double pf_lin_x_vel_prev_error = 0.0;
  double pf_lin_x_vel_error_sum = 0.0;
  double pf_lin_y_vel_prev_error = 0.0;
  double pf_lin_y_vel_error_sum = 0.0;
  double pf_ang_z_vel_prev_error = 0.0;
  double pf_ang_z_vel_error_sum = 0.0;

  double plat_vel_setpoint[3] = {0.1, 0.1, 0.5};
  double plat_clip_force = 20.0;
  double plat_sat_force = 300.0;

  double pivot_vel_zero_threshold = 4.5;
  double pivot_vel_ramp_factor = 0.1;
  double ff_tau_c = 5.0;
  double current_ff_tau_c[4] = {0.0, 0.0, 0.0, 0.0};
  double plat_vel_damping_tube = 0.001;
  double plat_vel_damping_factor = 0.1;
  double plat_vel_attenuation_factor = 0.2;
  double pivot_align_error_margin = 0.25;

  double plat_vel_xy_pid_controller_kp = 200.0;
  double plat_vel_xy_pid_controller_ki = 0.0;
  double plat_vel_xy_pid_controller_kd = 0.0;

  double plat_vel_ang_pid_controller_kp = 100.0;
  double plat_vel_ang_pid_controller_ki = 0.0;
  double plat_vel_ang_pid_controller_kd = 0.0;

  double plat_vel_x_prev_error = 0.0;
  double plat_vel_y_prev_error = 0.0;
  double plat_vel_ang_prev_error = 0.0;

  double plat_vel_x_error_sum = 0.0;
  double plat_vel_y_error_sum = 0.0;
  double plat_vel_ang_error_sum = 0.0;

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
    printf("count: %d\n", count);

    get_robot_data(&robot, *control_loop_dt);

    // printf("odometry: ");
    // print_array(robot.mobile_base->state->x_platform, 3);

    double plat_force[3] = {pf[0], pf[1], pf[2]};  // [N], [N], [Nm]

    // damping controller to regulate the platform velocity
    double plat_vel_damping_force[3] = {0.0, 0.0, 0.0};
    double plat_vel_error[3] = {0.0, 0.0, 0.0};

    // check if platform velocity is greater than the setpoint
    for (int i = 0; i < 3; ++i)
    {
      if (plat_force[i] > plat_clip_force &&
          robot.mobile_base->state->xd_platform[i] > plat_vel_setpoint[i])
        computeEqualityError(robot.mobile_base->state->xd_platform[i], plat_vel_setpoint[i],
                             plat_vel_error[i]);
      else if (plat_force[i] < -plat_clip_force &&
               robot.mobile_base->state->xd_platform[i] < -plat_vel_setpoint[i])
        computeEqualityError(robot.mobile_base->state->xd_platform[i], -plat_vel_setpoint[i],
                             plat_vel_error[i]);
      else
        plat_vel_error[i] = 0.0;
    }

    // PID controller for regulating the platform velocity
    pidController(plat_vel_error[0], plat_vel_xy_pid_controller_kp, plat_vel_xy_pid_controller_ki,
                  plat_vel_xy_pid_controller_kd, *control_loop_dt, plat_vel_x_error_sum, 5.0,
                  plat_vel_x_prev_error, plat_vel_damping_force[0]);
    pidController(plat_vel_error[1], plat_vel_xy_pid_controller_kp, plat_vel_xy_pid_controller_ki,
                  plat_vel_xy_pid_controller_kd, *control_loop_dt, plat_vel_y_error_sum, 5.0,
                  plat_vel_y_prev_error, plat_vel_damping_force[1]);
    pidController(plat_vel_error[2], plat_vel_ang_pid_controller_kp,
                  plat_vel_ang_pid_controller_ki, plat_vel_ang_pid_controller_kd, *control_loop_dt,
                  plat_vel_ang_error_sum, 5.0, plat_vel_ang_prev_error, plat_vel_damping_force[2]);

    // adjust the platform force based on the damping controller
    for (size_t i = 0; i < 3; i++)
    {
      if (fabs(plat_vel_error[i]) > plat_vel_damping_tube)
      {
        plat_force[i] += plat_vel_damping_force[i];

        // Dynamically reduce force to prevent overshooting
        if ((plat_force[i] > 0.0 && plat_vel_error[i] < 0.0) ||
            (plat_force[i] < 0.0 && plat_vel_error[i] > 0.0))
        {
          plat_force[i] *= 1.0 - fabs(plat_vel_error[i]) * plat_vel_damping_factor;
        }
      }
      else if (plat_force[i] < plat_clip_force && plat_force[i] > -plat_clip_force)
      {
        // Smooth clipping adjustment
        plat_force[i] *= (1.0 - plat_vel_attenuation_factor);
      }
    }

    // saturation
    for (size_t i = 0; i < 3; i++)
    {
      if (plat_force[i] > plat_sat_force)
      {
        plat_force[i] = plat_sat_force;
      }
      else if (plat_force[i] < -plat_sat_force)
      {
        plat_force[i] = -plat_sat_force;
      }
    }

    printf("platform force: ");
    print_array(plat_force, 3);

    printf("platform velocity: ");
    print_array(robot.mobile_base->state->xd_platform, 3);

    double lin_offsets[4];
    double ang_offsets[4];
    get_pivot_alignment_offsets(&robot, plat_force, lin_offsets, ang_offsets);

    // transform the platform force by 90 degrees ccw
    Eigen::Rotation2Dd pf_correction_rot(M_PI / 2);
    Eigen::Vector2d lin_pf = Eigen::Vector2d(plat_force[0], plat_force[1]);

    // compute the weights for the platform force
    double platform_weights[2];
    platform_weights[0] =
        abs(plat_force[2]) < 1e-6
            ? 1.0
            : sqrt(pow(plat_force[0], 2) + pow(plat_force[1], 2)) /
                  (sqrt(pow(plat_force[0], 2) + pow(plat_force[1], 2) + pow(plat_force[2], 2)));

    platform_weights[1] = 1.0 - platform_weights[0];

    double lin_force_weight = lin_pf.norm() == 0.0 ? 0.0 : platform_weights[0];
    double moment_weight = plat_force[2] == 0.0 ? 0.0 : platform_weights[1];

    double tau_align[4]{};
    double dist_align[4]{};

    for (size_t i = 0; i < robot.mobile_base->mediator->kelo_base_config->nWheels; i++)
    {
      dist_align[i] = lin_offsets[i] * lin_force_weight + ang_offsets[i] * moment_weight;
    }

    double dist_align_errors[4]{};

    for (size_t i = 0; i < robot.mobile_base->mediator->kelo_base_config->nWheels; i++)
    {
      computeEqualityError(dist_align[i], 0.0, dist_align_errors[i]);
      if (abs(dist_align_errors[i]) > pivot_align_error_margin)
        pidController(dist_align_errors[i], Kp, Ki, Kd, control_loop_timestep, w_dist_error_sum[i],
                      20.0, w_dist_prev_error[i], tau_align[i]);
    }

    // base solver
    double tau_wheel_c[8]{};
    // base_fd_solver_with_alignment(&robot, plat_force, lin_offsets, ang_offsets, tau_wheel_c);
    base_fd_solver_cgls(&robot, plat_force, tau_align, tau_wheel_c);

    // ramp the ff torques based on the pivot velocities
    for (size_t i = 0; i < 4; i++)
    {
      if (abs(robot.mobile_base->state->pivot_velocities[i]) < pivot_vel_zero_threshold &&
          abs(tau_align[i]) > 0.0)
      {
        // Ramp the ff_tau_c value up
        if (current_ff_tau_c[i] < ff_tau_c)
        {
          current_ff_tau_c[i] += pivot_vel_ramp_factor;
          if (current_ff_tau_c[i] > ff_tau_c)
            current_ff_tau_c[i] = ff_tau_c;
        }
      }
      else
      {
        // Ramp the ff_tau_c value down
        if (current_ff_tau_c[i] > 0.0)
        {
          current_ff_tau_c[i] -= pivot_vel_ramp_factor;
          if (current_ff_tau_c[i] < 0.0)
            current_ff_tau_c[i] = 0.0;
        }
      }

      tau_wheel_c[2 * i] +=
          dist_align_errors[i] < 0.0 ? -current_ff_tau_c[i] : current_ff_tau_c[i];
      tau_wheel_c[2 * i + 1] +=
          dist_align_errors[i] < 0.0 ? -current_ff_tau_c[i] : current_ff_tau_c[i];
    }

    // saturate torques
    double tau_limit = 10.0;
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

    double wtau[8];
    for (size_t i = 0; i < 4; i++)
    {
      wtau[2 * i] = tau_wheel_c[2 * i];
      wtau[2 * i + 1] = tau_wheel_c[2 * i + 1];
    }

    // printf("wheel taus:  ");
    // print_array(wtau, 8);

    printf("\n");

    if (count > 2)
    {
      // raise(SIGINT);
      set_mobile_base_torques(&robot, wtau);
      update_base_state(robot.mobile_base->mediator->kelo_base_config,
                        robot.mobile_base->mediator->ethercat_config);
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