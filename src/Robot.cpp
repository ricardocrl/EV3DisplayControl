/*
 * Robot.cpp
 *
 *  Created on: 02/10/2017
 *      Author: roliveira
 */

#include <iostream>
#include <unistd.h>
#include <cmath>
#include <algorithm>

#include "Robot.h"

#define STUD_MM (7.93f)
#define PI (3.141596f)

Robot::Robot() :
        armLength_mm(1), armOffset_mm(
                std::make_pair((int) (11.5 * STUD_MM), (int) (-1.5 * STUD_MM))), wheelRadius(2.6 * STUD_MM) {

    clickDispatchThread = std::thread(std::bind(&Robot::clickDispatchLoop, this));
    clickOngoing.store(false);
}

Robot::~Robot() {
    //goHome();
	if (isOpen()) {
		std::cout << "Stopping motors..." << std::endl;
		outputStop(outX, StopLevel::FLOAT);
		outputStop(outArm, StopLevel::FLOAT);
		close();
	}
}

bool Robot::connect(const std::string & devPath) {
    bool ret;
    if (devPath.empty()) {
        ret = open();
    } else {
        ret = open(devPath);
    }
    if (ret) {
        outputReset(Output::ABCD);
        outputSetRamp(outX, 30, 30);
        outputSetRamp(outArm, 30, 40);
        outputSetRamp(outClick, 0, 0);
    }
    return ret;
}

bool Robot::isConnected() {
    return isOpen();
}

bool Robot::goHome() {
    if (!isConnected()) {
        std::cout << "Robot::goHome error - robot not connected" << std::endl;
        return false;
    }

    if (!getTouch(inTouch)) {
        outputSpeed(outX, -outXSpeed*outXPolarity);
    }
    outputPower(outArm, (int)(-outArmSpeed*outArmPolarity*1.5));
    usleep(200000);

    bool armHome = false;
    bool xHome = false;

    while (!armHome || !xHome) {
        usleep(10000);
        std::cout << "arm speed: " << getSpeed(outArm) << std::endl;
        std::cout << "touch x: " << getTouch(inTouch) << std::endl;

        if (!armHome) {
            armHome = (getSpeed(outArm) <= 1);
            if (armHome) {
                outputStop(outArm, StopLevel::BRAKE);
                std::cout << "tacho before adjust: " << getTacho(outArm) << std::endl;
                outputPowerStep(outArm, 5, 28*outArmPolarity, StopLevel::BRAKE);
                std::cout << "tacho after adjust: " << getTacho(outArm) << std::endl;
            }
        }

        if (!xHome) {
            xHome = getTouch(inTouch);
            if (xHome) {
                outputStop(outX, StopLevel::BRAKE_AND_FLOAT);
            }
        }
    }
    usleep(200000);
    outputReset(outArm);
    outputReset(outX);

    driveTo({0, 0});
    clickXY(0, 0);
    return true;
}

int Robot::getTachoDiff(const Output output, const int targetTacho) {
    if (output == outArm) {
        // Due to the pendulum effect, we shall subtract the backlash effect
        // depending on the side the pendulum is leaning to
        int targetWithBacklash = targetTacho;
        if (targetTacho < 89) {
            targetWithBacklash -= static_cast<int>(std::abs(cos(targetTacho * PI / 180)) * outArmBacklash);
        } else if (targetTacho > 91) {
            targetWithBacklash += static_cast<int>(std::abs(cos(targetTacho * PI / 180)) * outArmBacklash);
        }

        return (targetWithBacklash)*outArmPolarity - getTacho(outArm);
    } else if (output == outX) {
        return (targetTacho)*outXPolarity - getTacho(outX);
    } else {
        std::cout << "Robot::getTicksDistance Output not implemented" << std::endl;
        return 0;
    }
}

int Robot::chooseCandidate(const std::vector<std::pair<int, int>> candidates) {
    std::cout << "candidates:" << std::endl;
    for (auto const cand : candidates) {
        std::cout << cand.first << ", " << cand.second << std::endl;
    }
    if (candidates.empty() || candidates.size() > 2) {
        return -1;
    } else if (candidates.size() == 1) {
        return 0;
    } else {
        std::vector<bool> good(2, false);
        for (size_t i = 0; i < good.size(); ++i) {
            if ((candidates[i].first > 0) && (candidates[i].second >= r_pos_min)
                    && (candidates[i].second <= r_pos_max)) {
                good[i] = true;
            }
        }
        if (std::all_of(good.begin(), good.end(), [](bool g) { return !g; })) {
            return -1;
        } else {
            float minExpectedTime = 100.0;
            int idxWinner = 0;

            for (size_t i = 0; i < good.size(); ++i) {
                if (good[i]) {

                    for (auto const & output : { outArm, outX }) {
                        float expectTime;
                        int outSpeed, tachoDist, targetTacho;

                        if (output == outArm) {
                            outSpeed = outArmSpeed;
                            targetTacho = candidates[i].second;
                        } else {
                            outSpeed = outXSpeed;
                            targetTacho = candidates[i].first;
                        }
                        tachoDist = std::abs(getTachoDiff(output, targetTacho)) / (outSpeed * ticksPerSecFactor);
                        expectTime = tachoDist / (outSpeed * ticksPerSecFactor);

                        if (expectTime < minExpectedTime) {
                            idxWinner = i;
                            minExpectedTime = expectTime;
                        }
                    }
                }
            }
            return idxWinner;
        }
    }
}

bool Robot::driveTo(std::pair<int, int> motorsTargetPos) {
    std::cout << "Robot::driveTo " << motorsTargetPos.first << ", " << motorsTargetPos.second << std::endl;
    std::cout << "Robot::driveTo Dist: " << getTachoDiff(outX, motorsTargetPos.first) << ", "
            << getTachoDiff(outArm, motorsTargetPos.second) << std::endl;
    int steps = getTachoDiff(outX, motorsTargetPos.first);
    if (steps != 0) {
        std::cout << "Driving X steps: " << steps << std::endl;
        std::cout << "Robot::driveTo tacho x: " << getTacho(outX) << std::endl;
        outputSpeedStep(outX, outXSpeed, steps, StopLevel::BRAKE_AND_FLOAT, Layer::L0, false);
    }
    steps = getTachoDiff(outArm, motorsTargetPos.second);
    if (steps != 0) {
        std::cout << "Driving Arm steps: " << steps << std::endl;
        std::cout << "Robot::driveTo tacho arm: " << getTacho(outArm) << std::endl;
        outputSpeedStep(outArm, outArmSpeed, steps, StopLevel::BRAKE, Layer::L0, false);
    }
    while(!outputReady(outX) || !outputReady(outArm)) {
        usleep(5000);
    }
    std::cout << "Robot::driveTo tacho arm: " << getTacho(outArm) << std::endl;
    return true;
}

bool Robot::goTo(const int x, const int y) {
    std::cout << "Robot::goTo " << x << ", " << y << std::endl;
    float const & edgeHypothenuse = armLength_mm;
    float edgeAdjacent = y - armOffset_mm.second;

    if (edgeAdjacent >= edgeHypothenuse) {
        playTone(1, 800, 500);
        return false;
    }

    float edgeOpposite = sqrt(pow(edgeHypothenuse, 2) - pow(edgeAdjacent, 2));

    std::cout << "Triang (h,o,a):" << edgeHypothenuse << " " << edgeOpposite << " " << edgeAdjacent << std::endl;
    float angle = asin((edgeOpposite) / edgeHypothenuse);
    float angle_d = (angle / (2 * PI)) * 360;
    std::cout << "Angle degrees: " << angle_d << std::endl;

    // 2 pairs <tachoCount_outX, tachoCount_outArm>
    std::vector<std::pair<int, int>> candidateMotorsPos(2);
    candidateMotorsPos[0] = {static_cast<int>(360 * (x - edgeOpposite) / (2*PI*wheelRadius)), static_cast<int>(90 + angle_d)};
    candidateMotorsPos[1] = {static_cast<int>(360 * (x + edgeOpposite) / (2*PI*wheelRadius)), static_cast<int>(90 - angle_d)};

    int idx = chooseCandidate(candidateMotorsPos);
    if (idx >= 0) {
        return driveTo(candidateMotorsPos[idx]);
    } else {
        playTone(1, 800, 500);
        return false;
    }
}

bool Robot::clickXY(const int x, const int y) {
    dispatchMtx.lock();
    dispatcherQueue.push(std::bind(&Robot::clickDispatch, this, x, y));
    dispatchMtx.unlock();
    return true;
}

void Robot::clickDispatchLoop() {
    while (true) {
        dispatchMtx.lock();
        if (!dispatcherQueue.empty() && !clickOngoing.load()) {
            auto fnc = dispatcherQueue.front();
            dispatcherQueue.pop();
            dispatchMtx.unlock();

            clickOngoing.store(true);
            fnc();
            clickOngoing.store(false);
        } else {
            dispatchMtx.unlock();
        }
        usleep(2000);
    }
}

bool Robot::clickDispatch(const int x, const int y) {
    bool success;
    success = goTo(x, y);
    if (success) {
        success = fingerClick();
    }
    return success;
}

bool Robot::fingerClick() {
    return outputPowerStep(outClick, 100, 360, StopLevel::BRAKE_AND_FLOAT);
}

void Robot::setArmLengthMm(const unsigned int length_mm) {
    armLength_mm = length_mm;
}

int Robot::getArmLengthMm() const {
    return armLength_mm;
}

