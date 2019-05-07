#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include <zcm/zcm-cpp.hpp>

#include <mavlink/v2.0/common/mavlink.h>
#include <Eigen/Eigen>
#include <common/math/LowPass.hpp>
#include <common/mavlink/CommunicationPort.hpp>

namespace maav
{
namespace mavlink
{
struct Messages
{
    mavlink_heartbeat_t heartbeat;
    mavlink_system_time_t system_time;
    mavlink_ping_t ping;
    mavlink_altitude_t altitude;
    mavlink_attitude_target_t attitude_target;
};

struct EmsState
{
    double z_velocity;
    uint64_t usec;
};

struct InnerLoopSetpoint
{
    Eigen::Quaternion<float> q{1, 0, 0, 0};
    float thrust = 0;      // 0 <= thrust <= 1
    float roll_rate = 0;   // rad/s
    float pitch_rate = 0;  // rad/s
    float yaw_rate = 0;    // rad/s

    static InnerLoopSetpoint zero() { return {{1, 0, 0, 0}, 0, 0, 0, 0}; }
};

class OffboardControl
{
public:
    OffboardControl(const CommunicationType, const std::string& port_path = "");
    ~OffboardControl();
    void init(std::atomic<bool>& kill);

    void set_attitude_target(const InnerLoopSetpoint&, const uint8_t = 0b00000000);
    bool arm();
    bool readMessage();
    bool disarm();
    bool is_armed();
    bool check_offboard_control();
    bool offboardControlActive();
    bool activate_offboard_control();
    void holdZeroAttitude(const std::chrono::seconds hold_period);

private:
    void read_thread();
    void write_message(const mavlink_message_t& message);

    void ping(const uint64_t boot_timestamp);
    void send_heartbeat();
    bool arm_disarm(const bool arm);

    Messages current_messages_in;
    std::thread read_tid;
    std::thread write_tid;
    CommunicationPort com_port;

    bool offboard_control_active;
    bool armed;

    LowPass alt_filter;
    LowPass vel_filter;
    double altitude_last;
    double usec_last;
    double z_velocity_last;
    double dt;
    EmsState ems_state;

    zcm::ZCM zcm;

    // These are defined so that they are not magic numbers in the code
    const uint8_t system_id = 1;     // system we are connecting should always be 1 (only system)
    const uint8_t autopilot_id = 1;  // componenet we are controlling should always be 1 (autopilot)
    const uint8_t companion_id = 2;
    const uint32_t custom_mode = 393216;  // from px4_custom_mode.h in px4 firmware
    const uint8_t armed_base_mode = 157;

    std::atomic<bool>* kill_ = nullptr;
};

}  // maav
}  // gnc