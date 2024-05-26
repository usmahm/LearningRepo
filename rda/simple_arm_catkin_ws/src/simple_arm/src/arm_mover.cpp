#include "ros/ros.h"
#include "simple_arm/GoToPosition.h"
#include <std_msgs/Float64.h>

class AMSubscribeAndPublish {
  public:
    AMSubscribeAndPublish() {
      // Define two publishers to publish std_msgs::Float64 messages on joints respective topics
      joint1_pub_ = n_.advertise<std_msgs::Float64>("/simple_arm/joint_1_position_controller/command", 10);
      joint2_pub_ = n_.advertise<std_msgs::Float64>("/simple_arm/joint_2_position_controller/command", 10);

      // Define a safe_move service_ with a handle_safe_move_request callback function
      service_ = n_.advertiseService("/arm_mover/safe_move", &AMSubscribeAndPublish::handle_safe_move_request, this);
    }

    ros::NodeHandle getNode() const {
      return n_;
    }

    std::vector<float> clamp_at_boundaries(float requested_j1, float requested_j2) {
      // Define clamped joint angles and assign them to the requested ones
      float clamped_j1 = requested_j1;
      float clamped_j2 = requested_j2;

      // Get min and max joint parameters, and assign them to their respective variables
      float min_j1, max_j1, min_j2, max_j2;
      
      std::string node_name = ros::this_node::getName();

      // Get joints min and mx parameters
      n_.getParam(node_name + "/min_joint_1_angle", min_j1);
      n_.getParam(node_name + "/max_joint_1_angle", max_j1);
      n_.getParam(node_name + "/min_joint_2_angle", min_j2);
      n_.getParam(node_name + "/max_joint_2_angle", max_j2);
      
      // Check if joint 1 falls in the safe zone, otherwise clamp it
      if (requested_j1 < min_j1 || requested_j1 > max_j1) {
        clamped_j1 = std::min(std::max(requested_j1, min_j1), max_j1);
        ROS_WARN("j1 is out of bounds, valid range (%1.2f,%1.2f), clamping to: %1.2f", min_j1, max_j1, clamped_j1);
      } 

      // Check if joint 2 falls in the safe zone, otherwise clamp it
      if (requested_j2 < min_j2 || requested_j2 > max_j2) {
        clamped_j2 = std::min(std::max(requested_j2, min_j2), max_j2);
        ROS_WARN("j2 is out of bounds, valid range (%1.2f,%1.2f), clamping to: %1.2f", min_j2, max_j2, clamped_j2);
      }
      
      // Store clamped joint angles in a clamped_data vector
      std::vector<float> clamped_data = {clamped_j1, clamped_j2};
      return clamped_data;
    }

    bool handle_safe_move_request(simple_arm::GoToPosition::Request& req, simple_arm::GoToPosition::Response& res) {
      ROS_INFO("GoToPositionRequest received - j1:%1.2f, j2:%1.2f", (float)req.joint_1, (float)req.joint_2);

      auto joint_angles = clamp_at_boundaries(req.joint_1, req.joint_2);

      // Publish clamped message to the arm
      std_msgs::Float64 joint1_angle, joint2_angle;

      joint1_angle.data = joint_angles[0];
      joint2_angle.data = joint_angles[1];

      joint1_pub_.publish(joint1_angle);
      joint2_pub_.publish(joint2_angle);

      ros::Duration(3).sleep();

      res.msg_feedback = "Joint angles set - j1: " + std::to_string(joint_angles[0]) + " , j2: " + std::to_string(joint_angles[1]);
      ROS_INFO_STREAM(res.msg_feedback);

      return true;
    }
  private:
    ros::NodeHandle n_;
    ros::ServiceServer service_;
    ros::Publisher joint1_pub_, joint2_pub_;
};

int main(int argc, char** argv) {
  // Initialize arm_mover and create a handle to it
  ros::init(argc, argv, "arm_mover");

  AMSubscribeAndPublish SAPObject;

  // Handle ROS communication events and blocks untill shutdown reques is received by node.
  ros::spin();

  return 0;
}