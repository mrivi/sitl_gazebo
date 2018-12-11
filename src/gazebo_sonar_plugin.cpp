/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
/*
 * Desc: Contact plugin
 * Author: Nate Koenig mod by John Hsu
 */

#include "gazebo/physics/physics.hh"
#include "gazebo_sonar_plugin.h"

#include <gazebo/common/common.hh>
#include <gazebo/common/Plugin.hh>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include "gazebo/transport/transport.hh"
#include "gazebo/msgs/msgs.hh"

#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdio.h>


using namespace gazebo;
using namespace std;

// Register this plugin with the simulator
GZ_REGISTER_SENSOR_PLUGIN(SonarPlugin)

/////////////////////////////////////////////////
SonarPlugin::SonarPlugin()
{
  printf("constructor \n");
}

/////////////////////////////////////////////////
SonarPlugin::~SonarPlugin()
{
  this->parentSensor.reset();
  this->world.reset();
}

/////////////////////////////////////////////////
void SonarPlugin::Load(sensors::SensorPtr _parent, sdf::ElementPtr _sdf)
{
//  Get then name of the parent sensor
  this->parentSensor = std::dynamic_pointer_cast<sensors::SonarSensor>(_parent);

  if (!this->parentSensor)
    gzthrow("SonarPlugin requires a Sonar Sensor as its parent");

  this->world = physics::get_world(this->parentSensor->WorldName());

  this->parentSensor->SetActive(false);
  this->newScansConnection = this->parentSensor->ConnectUpdated(boost::bind(&SonarPlugin::OnNewScans, this));
  this->parentSensor->SetActive(true);

  if (_sdf->HasElement("robotNamespace"))
    namespace_ = _sdf->GetElement("robotNamespace")->Get<std::string>();
  else
    gzwarn << "[gazebo_sonar_plugin] Please specify a robotNamespace.\n";

  node_handle_ = transport::NodePtr(new transport::Node());
  node_handle_->Init(namespace_);

  const string scopedName = _parent->ParentName();
  string topicName = "~/" + scopedName + "/sonar";
  boost::replace_all(topicName, "::", "/");

  topic_name_vector_.push_back(topicName);
  topic_name_vector_.push_back("~/typhoon_h480/sonar2_model/link/sonar");
   topic_name_vector_.push_back("~/typhoon_h480/sonar_right_model/link/sonar");

 
  for (int i = 0; i < topic_name_vector_.size(); ++i)
  {
  // gzwarn << topic_name_vector_[i] << " " << i << "\n";
   sonar_pub_[i] = node_handle_->Advertise<sensor_msgs::msgs::Range>(topic_name_vector_[i], 10);
  }
  
}

void SonarPlugin::OnNewScans()
{
  // Get the current simulation time.
#if GAZEBO_MAJOR_VERSION >= 9
  common::Time now = world->SimTime();
#else
  common::Time now = world->GetSimTime();
#endif

  sonar_message.set_time_usec(now.Double() * 1e6);
  sonar_message.set_min_distance(parentSensor->RangeMin());
  sonar_message.set_max_distance(parentSensor->RangeMax());
  sonar_message.set_current_distance(parentSensor->Range());

  sonar_message.set_h_fov(2.0f * atan(parentSensor->GetRadius() / parentSensor->RangeMax()));
  sonar_message.set_v_fov(2.0f * atan(parentSensor->GetRadius() / parentSensor->RangeMax()));
  ignition::math::Quaterniond pose_model_quaternion = parentSensor->Pose().Rot();
  gazebo::msgs::Quaternion* orientation = new gazebo::msgs::Quaternion();
  orientation->set_x(pose_model_quaternion.X());
  orientation->set_y(pose_model_quaternion.Y());
  orientation->set_z(pose_model_quaternion.Z());
  orientation->set_w(pose_model_quaternion.W());
  sonar_message.set_allocated_orientation(orientation);
  sonar_message.set_id(pub_index_);

 // gzwarn << sonar_pub_[pub_index_]->GetTopic() << " pub_index " << pub_index_ << "\n";
  
  sonar_pub_[pub_index_]->Publish(sonar_message);

  pub_index_ = (pub_index_ >= (topic_name_vector_.size() -1)) ? 0 : (pub_index_ + 1);
  //gzwarn << pub_index_ << "\n";
}
