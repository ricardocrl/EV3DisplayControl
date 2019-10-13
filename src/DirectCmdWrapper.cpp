/*
 * DirectCmdWrapper.cpp
 *
 *  Created on: 10/10/2017
 *      Author: roliveira
 */

#include <unistd.h>
#include <fcntl.h>
#include <cmath>

#include "../ev3sources/lms2012/c_com/source/c_com.h"
#include "DirectCmdWrapper.h"

#define MAX_READ_SIZE 255

DirectCmdWrapper::DirectCmdWrapper() : fd(0) {
    rampupdown[Output::A] = { rampupdown_default, rampupdown_default };
    rampupdown[Output::B] = { rampupdown_default, rampupdown_default };
    rampupdown[Output::C] = { rampupdown_default, rampupdown_default };
    rampupdown[Output::D] = { rampupdown_default, rampupdown_default };
}

DirectCmdWrapper::~DirectCmdWrapper() {
    if (fd > 0) {
        ::close(fd);
    }
}

bool DirectCmdWrapper::isOpen() {
    return (fd > 0);
}

bool DirectCmdWrapper::open() {
    for (std::string path : {"/dev/hidraw0" /*, "/dev/rfcomm0" */}) {
        for (int i = 0; i < 10; ++i) {
            if (open(path)) {
                return true;
            }
            path.at(path.size()-1) += 1;
        }
    }
    return false;
}

bool DirectCmdWrapper::open(const std::string & path) {
    int newFd = ::open(path.data(), O_RDWR | O_SYNC | O_NOCTTY);
    if (newFd > 0) {
        LOG("DirectCmdWrapper::open success " << path << ": " << newFd);
        // if success, close previous connection
        if (fd > 0) {
            if (::close(fd) != 0) {
                LOG("DirectCmdWrapper::open could not close previous fd: " << fd);
            }
        }
        fd = newFd;
    } else {
        LOG("DirectCmdWrapper::open error " << path << ": " << newFd);
    }
    return isOpen();
}

bool DirectCmdWrapper::close() {
    if (fd > 0) {
        return (::close(fd) == 0);
    }
    return true;
}


bool DirectCmdWrapper::outputReset(const Output output, const Layer layer) {
    return (sendDirectCmd(opOUTPUT_CLR_COUNT, { (int) layer, (int) output }).isOk());
}

bool DirectCmdWrapper::outputReady(const Output output, const Layer layer) {
    return !(sendDirectCmd(opOUTPUT_TEST, {(int)layer, (int)output}, {1}).get<bool>(0));
}

bool DirectCmdWrapper::outputPowerStep(const Output output, const char power, const int step, const StopLevel stop,
        const Layer layer, const bool waitCompletion) {
    return outputStep(output, power, step, stop, layer, waitCompletion, opOUTPUT_STEP_POWER);
}

bool DirectCmdWrapper::outputSpeedStep(const Output output, const char speed, const int step, const StopLevel stop,
        const Layer layer, const bool waitCompletion) {
    return outputStep(output, speed, step, stop, layer, waitCompletion, opOUTPUT_STEP_SPEED);
}

bool DirectCmdWrapper::outputStep(const Output output, char outputVal, int steps, const StopLevel stop,
        const Layer layer, const bool waitCompletion, const unsigned char opcode) {
    if (steps < 0) {
        steps = -steps;
        outputVal = -outputVal;
    }
    std::vector<int> stepVec;
    calcStep123(output, steps, outputVal, stepVec);

    // Reset internal tacho. Why:
    // It seems that the firmware stores the value from the last movement
    // and makes a new move relative to that last position. If the user manually forced the movement
    // of the motor, that stored position is no longer valid. E.g. if the user turned 90 degrees,
    // and the new request 'step' is 360, the motor will only move for 270. Hence, we reset the value here.
    sendDirectCmd(opOUTPUT_RESET, { (int) layer, (int) output});

    // Request movement
    if (!sendDirectCmd(opcode, { (int) layer, (int) output, (int) outputVal, stepVec[0], stepVec[1],
            stepVec[2], (int)calcFirstStopLevel(stop) }).isOk()) {
        return false;
    }

    // Wait for completion if flag is true
    while (waitCompletion && !outputReady(output, layer)) usleep(10000);

    // Float if requested after a break
    if (waitCompletion && stop == StopLevel::BRAKE_AND_FLOAT) {
        floatAfterBrake(output, outputVal, layer);
    }
    return true;
}

bool DirectCmdWrapper::outputPower(const Output output, const char power, const Layer layer) {
    bool success = setPower(output, power, layer);
    if (success) {
        success = outputStart(output, layer);
    }
    return success;
}

bool DirectCmdWrapper::outputSpeed(const Output output, const char power, const Layer layer) {
    bool success = setSpeed(output, power, layer);
    if (success) {
        success = outputStart(output, layer);
    }
    return success;
}

bool DirectCmdWrapper::setPower(const Output output, const char power, const Layer layer) {
    return sendDirectCmd(opOUTPUT_POWER, {(int)layer, (int)output, (int)power}).isOk();
}

bool DirectCmdWrapper::setSpeed(const Output output, const char speed, const Layer layer) {
    return sendDirectCmd(opOUTPUT_SPEED, {(int)layer, (int)output, (int)speed}).isOk();
}

bool DirectCmdWrapper::outputStart(const Output output, const Layer layer) {
    return sendDirectCmd(opOUTPUT_START, {(int)layer, (int)output}).isOk();
}

bool DirectCmdWrapper::outputStop(const Output output, const StopLevel stop, const Layer layer) {
    switch (stop) {
        case StopLevel::BRAKE:
            return sendDirectCmd(opOUTPUT_STOP, {(int)layer, (int)output, (int)StopLevel::BRAKE}).isOk();
        case StopLevel::FLOAT:
            return sendDirectCmd(opOUTPUT_STOP, {(int)layer, (int)output, (int)StopLevel::FLOAT}).isOk();
        case StopLevel::BRAKE_AND_FLOAT:
            if (outputStop(output, StopLevel::BRAKE, layer)) {
                floatAfterBrake(output, 100, layer);
                return true;
            } else {
                return false;
            }
    }
    return false;
}

bool DirectCmdWrapper::setPolarity(const Output output, const Direction pol, const Layer layer) {
    return sendDirectCmd(opOUTPUT_POLARITY, {(int)layer, (int)output, (int)pol}).isOk();
}

int DirectCmdWrapper::getSpeed(const Output output, const Layer layer) {
    auto reply = sendDirectCmd(opOUTPUT_READ, {(int)layer, bitfieldToPortNo(output)}, {1, 4});
    if (reply.isOk()) {
        return static_cast<int>(reply.get<char>(0));
    } else {
        std::cout << "DirectCmdWrapper::getSpeed error - output: " << static_cast<int>(output) << " layer: "
                << static_cast<int>(layer) << std::endl;
        return 0;
    }
}

int DirectCmdWrapper::getTacho(const Output output, const Layer layer) {
    auto reply = sendDirectCmd(opINPUT_DEVICE, {READY_RAW, (int)layer, bitfieldToPortNo(output)+16, 0, 0, 1}, {4});
    if (reply.isOk()) {
        return reply.get<int>(0);
    } else {
        std::cout << "DirectCmdWrapper::getTacho error" << " output: " << static_cast<int>(output) << " layer: "
                << static_cast<int>(layer) << std::endl;
        return 0;
    }
}

bool DirectCmdWrapper::getTouch(const Input input, const Layer layer) {
    return static_cast<bool>(readSensorPercent(input, layer));
}

unsigned char DirectCmdWrapper::readSensorPercent(const Input input, const Layer layer) {
    auto reply = sendDirectCmd(opINPUT_READ, {(int)layer, (int)input, 16, 0}, {1});
    if (reply.isOk()) {
        return reply.get<unsigned char>(0);
    } else {
        std::cout << "DirectCmdWrapper::readSensorPercent error" << " input: " << static_cast<int>(input) << " layer: "
                << static_cast<int>(layer) << std::endl;
        return 0;
    }
}

bool DirectCmdWrapper::playTone(const unsigned char volume, const unsigned int tone_hz, const unsigned int ms,
        bool waitCompletion) {
    if(!sendDirectCmd(opSOUND, { TONE, (int) volume, (int) tone_hz, (int) ms }).isOk()) {

        std::cout << "DirectCmdWrapper::playTone error" << " volume: " << static_cast<int>(volume) << " tone_hz: "
                << static_cast<int>(tone_hz) << " ms: " << static_cast<int>(ms) << std::endl;
        return false;
    }
    while (waitCompletion && sendDirectCmd(opSOUND_TEST, {}, {1}).get<bool>(0)) usleep(10000);
    return true;
}

bool DirectCmdWrapper::setVolume(const unsigned char volume) {
    return sendDirectCmd(opINFO, {SET_VOLUME, (int)volume}).isOk();
}

DirectCmdReply DirectCmdWrapper::sendDirectCmd(unsigned char opcode, const std::vector<int>& params,
        const std::vector<size_t>& returnSizes, const bool waitReply) {
    DirectCmdReply reply;
    auto cmd = buildDirectCmd(opcode, params, returnSizes, waitReply);

    if (write(fd, cmd.data(), cmd[0] + 2) != cmd[0] + 2) {
    	return reply;
    }

    if (waitReply) {
        // 5 fixed bytes: { size0, size1, msgCount0, msgCount1, replyType }
        // byte 5 and 6: bytes allocated in the response buffer
        size_t readSizeExpected = 5 + cmd[5] + ((cmd[6] << 8) & 0xFF);

        unsigned char rsp[MAX_READ_SIZE];
        size_t readSizeActual = read(fd, rsp, readSizeExpected);

        if (readSizeActual != readSizeExpected) {
            LOG("Error reading msg");
        } else {
            reply = DirectCmdReply(rsp, readSizeActual, returnSizes);
        }
    }
    return reply;
}

std::vector<unsigned char> DirectCmdWrapper::buildDirectCmd(unsigned char opcode, const std::vector<int>& params,
        const std::vector<size_t>& returnSizes, const bool waitReply) {

    std::vector<unsigned char> cmd(2);  // First 2 bytes are the size of the message
                                        // which we will sort out later
    // Msg count
    cmd.push_back(0);
    cmd.push_back(0);

    // Reply type
    cmd.push_back(waitReply ? DIRECT_COMMAND_REPLY : DIRECT_COMMAND_NO_REPLY);

    // Variable allocation
    size_t totalSize = 0; // all returned values must be 4-byte aligned
    for (auto const size : returnSizes) {
        totalSize += ((size - 1) / 4 + 1) * 4;
    }
    cmd.push_back(totalSize & 0xFF);
    cmd.push_back((totalSize >> 8) & 0xFF);

    // First byte of byte codes, the opCode
    cmd.push_back(opcode);

    for (auto const & param : params) {
        std::vector<int> paramVec;

        if (param >= lc0_min && param <= lc0_max) {
            paramVec = { LC0(param) };
        } else if (param >= lc1_min && param <= lc1_max) {
            paramVec = { LC1(param) };
        } else if (param >= lc2_min && param <= lc2_max) {
            paramVec = { LC2(param) };
        } else if (param >= lc4_min && param <= lc4_max) {
            unsigned long lparam[5] = { LC4(param) };
            for (int i = 0; i < 5; ++i) {
                paramVec.push_back(static_cast<unsigned char>(lparam[i]));
            }
        }
        cmd.insert(cmd.end(), paramVec.begin(), paramVec.end());
    }

    // Compose cmd with global variable indexes for returns
    int varIdx = 0;
    for (auto const & size : returnSizes) {
        std::vector<int> paramVec;

        if (varIdx >= lc0_min && varIdx <= lc0_max) {
            paramVec = { GV0(varIdx) };
        } else if (varIdx >= lc1_min && varIdx <= lc1_max) {
            paramVec = { GV1(varIdx) };
        } else if (varIdx >= lc2_min && varIdx <= lc2_max) {
            paramVec = { GV2(varIdx) };
        } else if (varIdx >= lc4_min && varIdx <= lc4_max) {
            paramVec = { GV4(varIdx) };
        }
        cmd.insert(cmd.end(), paramVec.begin(), paramVec.end());
        varIdx = (((varIdx + size) - 1) / 4 + 1) * 4;
    }

    // Fill in size bytes
    cmd[0] = (cmd.size() - 2) & 0xFF;
    cmd[1] = ((cmd.size() - 2) >> 8) & 0xFF;
    return cmd;
}

unsigned char DirectCmdWrapper::bitfieldToPortNo(const Output bf) {
    switch (bf) {
        case Output::A:
            return 0;
        case Output::B:
            return 1;
        case Output::C:
            return 2;
        case Output::D:
            return 3;
        default:
            std::cout << "DirectCmdWrapper::bitfieldToPortNo cannot convert multiple outputs to 1 port no."
                    << std::endl;
            return ~0;
    }
}

void DirectCmdWrapper::outputSetRamp(const Output motor, const int rampup, const int rampdown) {
    rampupdown[motor] = {rampup, rampdown};
}

void DirectCmdWrapper::calcStep123(const Output motor, const int fullSteps, char & power,
        std::vector<int> & steps) {
    if (rampupdown[motor].first + rampupdown[motor].second > fullSteps) {
        float coverage = (float)fullSteps / (rampupdown[motor].first + rampupdown[motor].second);
        power = std::max(static_cast<char>(std::abs(power) * coverage), rampMinCruisePower) * std::copysign(1.0, power);
        int rampup = rampupdown[motor].first * coverage;
        int rampdown = fullSteps - rampup;

        steps.push_back(rampup);
        steps.push_back(0);
        steps.push_back(rampdown);
    } else {
        steps.push_back(rampupdown[motor].first);
        steps.push_back(fullSteps - rampupdown[motor].first - rampupdown[motor].second);
        steps.push_back(rampupdown[motor].second);
    }
}

StopLevel DirectCmdWrapper::calcFirstStopLevel(const StopLevel stop) {
    if (stop == StopLevel::BRAKE_AND_FLOAT) {
        return StopLevel::BRAKE;
    } else {
        return stop;
    }
}

void DirectCmdWrapper::floatAfterBrake(const Output output, const char formerSpeed, const Layer layer) {
    int i = 0;
    const int delta = std::min(100000, std::abs((int)formerSpeed)*2000);
    int currTacho = getTacho(output);
    while (true) {
        usleep(delta);
        ++i;
        int newTacho = getTacho(output);
        if (newTacho == currTacho) {
            break;
        } else {
            currTacho = newTacho;
        }
    }
    outputStop(output, StopLevel::FLOAT, layer);
}

void DirectCmdWrapper::testInputDevice(Output output) {
    outputPower(output, 20);
    for (int port = 0; port < 30; ++port) {
        DirectCmdReply reply = sendDirectCmd(opINPUT_DEVICE, {GET_NAME, 0, port, 16}, {16});
        if (reply.isOk() && reply.get<std::string>(0) != "NONE") {
            LOG("Port " << port << ": " << reply.get<std::string>(0));
            for (int mode = 0; mode < 30; mode++) {
                reply = sendDirectCmd(opINPUT_DEVICE, {GET_MODENAME, 0, port, mode, 16}, {16});
                auto modeName = reply.get<std::string>(0);
                if (reply.isOk() && !modeName.empty()) {
                    reply = sendDirectCmd(opINPUT_DEVICE, {READY_RAW, (int)0, port, 0, mode, 1}, {4});
                    int val;
                    if (reply.isOk()) {
                        val = reply.get<int>(0);
                        LOG("Mode " << mode << ": " << modeName << " - current val: " << val);
                    }
                }
            }
            std::cout << std::endl;
        }
        usleep(10000);
    }
    outputStop(output, StopLevel::FLOAT);
}
