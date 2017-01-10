#pragma once

#include <eigen3/Eigen/Eigen>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Vector3.h>

#include <vector>

class WeightedMean
{
public:
    WeightedMean();

    void addVec3(const tf2::Vector3& vec, double weight);
    void addQuat(const tf2::Quaternion& quat, double weight);
    void clear();

    tf2::Vector3 weightedMeanVec3() const;
    tf2::Quaternion weightedMeanQuat() const;

private:
    Eigen::Vector3d m_vectors = { 0, 0, 0 };
    Eigen::Matrix4Xd m_quats;

    double m_vectorWeights = 0.0;
};

class PassThroughFilter
{
public:
    void addVec3(const tf2::Vector3& vec);
    void addQuat(const tf2::Quaternion& quat);

private:
    Eigen::Vector3d m_vectors = { 0, 0, 0 };
    Eigen::Matrix4Xd m_quats;
};
