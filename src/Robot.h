/*
 * Robot.h
 *
 *  Created on: 02/10/2017
 *      Author: roliveira
 */

#ifndef ROBOT_H_
#define ROBOT_H_

#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include "DirectCmdWrapper.h"

class Robot : public DirectCmdWrapper {
public:
    Robot();
    ~Robot();

    bool connect(const std::string & devPath = "");
    bool isConnected();

    bool goHome();
    bool fingerClick();
    bool clickXY(const int x, const int y);
    bool goTo(const int x, const int y);

    void setArmLengthMm(const unsigned int length_mm);
    int getArmLengthMm() const;

private:
    int chooseCandidate(const std::vector<std::pair<int, int>> candidates);
    int getTachoDiff(const Output output, int targetTacho);
    bool driveTo(std::pair<int, int> motorsPos);

    bool clickDispatch(const int x, const int y);
    void clickDispatchLoop();

    int armLength_mm;

    const std::pair<int, int> armOffset_mm; // mm
    const float wheelRadius;                // mm
    const float ticksPerSecFactor = 10.3;
    const int r_pos_max = 180;              // deg
    const int r_pos_min = 0;                // deg
    const int outXSpeed = 50;               // * 10.3 ticks (degrees) / sec [0 - 100]
    const int outArmSpeed = 35;             // * 10.3 ticks (degrees) / sec [0 - 100]
    const int outArmPolarity = -1;          // sign  {-1, 1}
    const int outXPolarity = -1;            // sign  {-1, 1}
    const int outArmBacklash = 12;          // deg

    const Output outX = Output::A;
    const Output outArm = Output::C;
    const Output outClick = Output::D;
    const Input inTouch = Input::S4;

    std::mutex dispatchMtx;
    std::atomic<bool> clickOngoing;
    std::queue<std::function<void()>> dispatcherQueue;
    std::thread clickDispatchThread;
};


#endif /* ROBOT_H_ */
