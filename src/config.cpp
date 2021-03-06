/*
 * ATLAS - Cooperative sensing
 * Copyright (C) 2017  Paul KREMER
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <angles/angles.h>
#include <ros/ros.h>
#include <yaml-cpp/yaml.h>

Config::Config(const std::string& filename)
{
    auto root = YAML::LoadFile(filename);
    parseRoot(root);
}

Config::Config()
{
}

void Config::loadFromString(const std::string& input)
{
    auto root = YAML::Load(input);
    parseRoot(root);
}

tf2::Transform Config::parseTransform(const YAML::Node& node) const
{
    if (!node)
        return tf2::Transform::getIdentity();

    // parse rotation
    tf2::Quaternion rot = tf2::Quaternion::getIdentity();
    if (node["rot"].size() == 4)
    {
        rot.setX(node["rot"][0].as<double>());
        rot.setY(node["rot"][1].as<double>());
        rot.setZ(node["rot"][2].as<double>());
        rot.setW(node["rot"][3].as<double>());
    }
    else if (node["rot"].size() == 3)
    {
        rot.setRPY( //
            angles::from_degrees(node["rot"][0].as<double>()), //
            angles::from_degrees(node["rot"][1].as<double>()), //
            angles::from_degrees(node["rot"][2].as<double>()) //
            );
    }
    else if (node["rot"].size() != 0)
    {
        ROS_WARN("Config: 'rot' is expected to have either 3 elements (YPR) or 4 elements (quaternion), got %i. Default is {0,0,0,1}",
            int(node["rot"].size()));
    }

    // parse position
    tf2::Vector3 origin;
    if (node["origin"].size() == 3)
    {
        origin.setX(node["origin"][0].as<double>());
        origin.setY(node["origin"][1].as<double>());
        origin.setZ(node["origin"][2].as<double>());
    }
    else if (node["origin"].size() != 0)
    {
        ROS_WARN("Config: 'origin' is expected to have 3 elements, got %i. Default is {0,0,0}",
            int(node["origin"].size()));
    }

    return tf2::Transform(rot, origin);
}

void Config::parseRoot(const YAML::Node& node)
{
    // sensor type conversion
    std::map<std::string, Sensor::Type> typeMap = {
        { "MarkerBased", Sensor::Type::MarkerBased },
        { "NonMarkerBased", Sensor::Type::NonMarkerBased }
    };

    if (!node)
        ROS_ERROR("Config: Document is empty");

    // quick sanity checks
    if (!node["entities"])
        ROS_WARN("Config: Cannot find 'entities'");
    if (!node["options"])
        ROS_WARN("Config: Cannot find 'options'");

    // load the entities
    for (const YAML::Node& entity : node["entities"])
    {
        Entity entityData;
        entityData.name = entity["entity"].as<std::string>("undefined");

        // load filter config
        entityData.filterConfig.alpha = entity["filterAlpha"].as<double>(0.1);

        // load the sensor data
        for (const auto& sensor : entity["sensors"])
        {
            Sensor sensorData;
            sensorData.name   = sensor["sensor"].as<std::string>("undefined");
            sensorData.topic  = sensor["topic"].as<std::string>("undefined");
            sensorData.type   = typeMap[sensor["type"].as<std::string>("MarkerBased")];
            sensorData.sigma  = sensor["sigma"].as<double>(1.0);
            sensorData.target = sensor["target"].as<std::string>("undefined");
            sensorData.transf = parseTransform(sensor["transform"]);

            // add the sensor
            entityData.sensors.push_back(sensorData);
        }

        // load the markers
        for (const auto& marker : entity["markers"])
        {
            Marker markerData;
            markerData.id     = marker["marker"].as<int>(-1);
            markerData.transf = parseTransform(marker["transform"]);

            entityData.markers.push_back(markerData);
        }

        // add the entity
        m_entities.push_back(entityData);
    }

    // load the options
    const auto options = node["options"];

    if (options)
    {
        m_options.dbgGraphFilename     = options["dbgDumpGraphFilename"].as<std::string>("");
        m_options.dbgGraphInterval     = options["dbgDumpGraphInterval"].as<double>(0);
        m_options.loopRate             = options["loopRate"].as<double>(60.0);
        m_options.decayDuration        = options["decayDuration"].as<double>(0.25);
        m_options.publishMarkers       = options["publishMarkers"].as<bool>(true);
        m_options.publishWorldSensors  = options["publishWorldSensors"].as<bool>(true);
        m_options.publishEntitySensors = options["publishEntitySensors"].as<bool>(true);
        m_options.publishPoseTopics    = options["publishPoseTopics"].as<bool>(true);
    }
}

Options Config::options() const
{
    return m_options;
}

std::vector<Entity> Config::entities() const
{
    return m_entities;
}

void Config::dump() const
{
    std::cout << "\n=== CONFIG ===\n";

    std::cout << "Options:\n";
    std::cout << "  loopRate: " << m_options.loopRate << "\n";
    std::cout << "  decayDuration: " << m_options.decayDuration << "\n";
    std::cout << "  dbgGraphFilename: " << m_options.dbgGraphFilename << "\n";
    std::cout << "  dbgGraphInterval: " << m_options.dbgGraphInterval << "\n";
    std::cout << "  publishMarkers: " << m_options.publishMarkers << "\n";
    std::cout << "  publishWorldSensors: " << m_options.publishWorldSensors << "\n";
    std::cout << "  publishEntitySensors: " << m_options.publishEntitySensors << "\n";
    std::cout << "  publishPoseTopics: " << m_options.publishPoseTopics << "\n";

    std::cout << "Entities:\n";
    for (const auto& entity : m_entities)
    {
        std::cout << "  -" << entity.name << "\n";
        std::cout << "    Sensors:\n";

        for (const auto& sensor : entity.sensors)
        {
            std::cout << "      -" << sensor.name << "\n";
        }

        std::cout << "    Markers:\n";

        for (const auto& marker : entity.markers)
        {
            std::cout << "      -ID:" << marker.id << "\n";
        }
    }

    std::cout << "=== CONFIG END ===\n";
    std::cout << std::endl;
}
