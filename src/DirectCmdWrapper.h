/*
 * DirectCmdWrapper.h
 *
 *  Created on: 10/10/2017
 *      Author: roliveira
 */

#ifndef DIRECTCMDWRAPPER_H_
#define DIRECTCMDWRAPPER_H_

#include <unordered_map>
#include "DirectCmdReply.h"

enum class Output : unsigned char {
    A = 0x01,
    B = 0x02,
    C = 0x04,
    D = 0x08,
    AB = A|B,
    AC = A|C,
    AD = A|D,
    BC = B|C,
    BD = B|D,
    CD = C|D,
    ABC = A|B|C,
    ABD = A|B|D,
    ACD = A|C|D,
    BCD = B|C|D,
    ABCD = A|B|C|D
};

enum class Input {
    S1 = 0,
    S2,
    S3,
    S4
};

enum class Layer {
    L0 = 0, /* your connected ev3 */
    L1,
    L2,
    L3
};

enum class Direction {
    BACKWARD = -1,
    REVERT = 0,
    FORWARD = 1
};

enum class StopLevel {
    FLOAT = 0,
    BRAKE = 1,
    BRAKE_AND_FLOAT = 2, /* Send a break command, followed by a float, to reduce battery consumption */
};

struct EnumClassHash {
    template<typename T>
    std::size_t operator()(T t) const {
        return static_cast<std::size_t>(t);
    }
};

class DirectCmdWrapper {
public:
    DirectCmdWrapper();
    ~DirectCmdWrapper();

    void testInputDevice(Output output);

    bool open(const std::string & path);
    bool open();
    bool isOpen();
    bool close();

    int getSpeed(const Output output, const Layer layer = Layer::L0);
    int getTacho(const Output output, const Layer layer = Layer::L0);
    bool getTouch(const Input input, const Layer layer = Layer::L0);

    bool setPolarity(const Output output, const Direction pol, const Layer layer = Layer::L0);
    bool outputReset(const Output output, const Layer layer = Layer::L0);

    void outputSetRamp(const Output motor, const int rampup, const int rampdown);
    bool outputReady(const Output output, const Layer layer = Layer::L0);
    bool outputPower(const Output output, const char power, const Layer layer = Layer::L0);
    bool outputPowerStep(const Output output, const char power, const int step, const StopLevel stop =
            StopLevel::BRAKE_AND_FLOAT, const Layer layer = Layer::L0, const bool waitCompletion = true);

    bool outputSpeed(const Output output, const char speed, const Layer layer = Layer::L0);
    bool outputSpeedStep(const Output output, const char speed, const int step, const StopLevel stop =
            StopLevel::BRAKE_AND_FLOAT, const Layer layer = Layer::L0, const bool waitCompletion = true);

    bool outputStop(const Output output, const StopLevel stop, const Layer layer = Layer::L0);

    bool playTone(const unsigned char volume, const unsigned int tone_hz, const unsigned int ms, bool waitCompletion = true);
    bool setVolume(const unsigned char volume);

    DirectCmdReply sendDirectCmd(unsigned char opcode, const std::vector<int>& params,
            const std::vector<size_t>& returnSizes = std::vector<size_t>(), const bool waitReply = true);

private:
    unsigned char readSensorPercent(const Input input, const Layer layer);
    bool setPower(const Output output, const char power, const Layer layer);
    bool setSpeed(const Output output, const char speed, const Layer layer);
    bool outputStart(const Output output, const Layer layer);
    void floatAfterBrake(const Output output, const char formerSpeed, const Layer layer);
    bool outputStep(const Output output, const char power, const int step, const StopLevel stop,
            const Layer layer, const bool waitCompletion, const unsigned char opcode);

    std::vector<unsigned char> buildDirectCmd(unsigned char opcode, const std::vector<int>& params,
            const std::vector<size_t>& returnSizes = std::vector<size_t>(), const bool waitReply = true);

    unsigned char bitfieldToPortNo(const Output bf);
    void calcStep123(const Output motor, const int fullSteps, char & power, std::vector<int> & steps);
    StopLevel calcFirstStopLevel(const StopLevel stop);

    const int lc0_max = 31;
    const int lc1_max = 127;
    const int lc2_max = 32767;
    const int lc4_max = 2147483647;
    const int lc0_min = -lc0_max;
    const int lc1_min = -lc1_max;
    const int lc2_min = -lc2_max;
    const int lc4_min = -lc4_max;
    const int rampupdown_default = 30;
    const char rampMinCruisePower = 10;

    std::unordered_map<Output, std::pair<int, int>, EnumClassHash> rampupdown;
    int fd;
};

#endif /* DIRECTCMDWRAPPER_H_ */
