#include "ros/ros.h"
#include "simple_arm/GoToPosition.h"
#include <sensor_msgs/JointState.h>
#include <sensor_msgs/Image.h>

class LASubscribeAndPublish {
  public:
    LASubscribeAndPublish() {
      // Define a client service capable of requesting services from safe_move
      client_ = n_.serviceClient<simple_arm::GoToPosition>("/arm_mover/safe_move");

      // Subscribe to /simple_arm/joint_states topic to read the arm joints position inside the joint_states_callback function
      sub1_ = n_.subscribe("/simple_arm/joint_states", 10, &LASubscribeAndPublish::joint_states_callback, this);

      // Subscribe to rgb_camera/image_raw topic to read the image data inside the look_away_callback function
      sub2_ = n_.subscribe("rgb_camera/image_raw", 10, &LASubscribeAndPublish::look_away_callback, this);
    }

    void look_away_callback(const sensor_msgs::Image img) {
      bool uniform_image = true;

      // Loop through each pixel in the image and check if its equal to the first one
      for (int i = 0; i < (img.height * img.step); i++) {
        if (img.data[i] - img.data[0] != 0) {
          uniform_image = false;
          break;
        }
      }

      // If the image is uniform and the arm is not moving, move the arm to the center
      if (uniform_image == true && moving_state_ == false)
        move_arm_center();
    }

    void joint_states_callback(const sensor_msgs::JointState js) {
      auto joints_current_position = js.position;

      double tolerance = 0.0005;

      // check if arm is moving
      if (fabs(joints_current_position[0] - joints_last_position_[0]) < tolerance && fabs(joints_current_position[1] - joints_last_position_[1]) < tolerance) {
        moving_state_ = false;
      } else {
        moving_state_ = true;
        joints_last_position_ = joints_current_position;
      }
    } 

    void move_arm_center() {
      ROS_INFO_STREAM("Moving the arm to the center");

      simple_arm::GoToPosition srv;
      srv.request.joint_1 = 1.57;
      srv.request.joint_2 = 1.57;

      // Call the safe_move service and pass the requested joint angles
      if (!client_.call(srv)) {
        ROS_ERROR("Failed to call service safe_move");
      }
    }

  private:
    ros::NodeHandle n_;
    ros::ServiceClient client_;
    ros::Subscriber sub1_;
    ros::Subscriber sub2_;
    std::vector<double> joints_last_position_{ 0, 0 };
    bool moving_state_ = false;
};

int main(int argc, char** argv) {
  // Initialize the look_away node and create a handle to it
  ros::init(argc, argv, "look_away");

  LASubscribeAndPublish SAPObject;

  // Handle ROS communication events
  ros::spin();

  return 0;
}