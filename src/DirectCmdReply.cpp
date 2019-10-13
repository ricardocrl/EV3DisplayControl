/*
 * DirectCmdReply.cpp
 *
 *  Created on: 10/10/2017
 *      Author: roliveira
 */

#include <algorithm>

#include "../ev3sources/lms2012/c_com/source/c_com.h"
#include "DirectCmdReply.h"

DirectCmdReply::DirectCmdReply(unsigned char * buf, const size_t size, const std::vector<size_t>& returnSizes) {
    /* total size = 2 (representing msg size) + msg size */
    size_t msgReplySize = buf[0] + (buf[1] << 8) + 2;

    /* reply type DIRECT_REPLY (0x02) or DIRECT_REPLY_ERROR (0x04) */
    if ((msgReplySize == size) && (buf[4] == DIRECT_REPLY)) {
        success = true;
    } else {
        success = false;
    }
    buf += 5;

    for (auto const & size : returnSizes) {
        size_t bufSize = ((size - 1) / 4 + 1) * 4;
        values.push_back(std::vector<unsigned char>(buf, buf + size));
        buf += bufSize;
    }
}

template <>
std::string DirectCmdReply::get<std::string>(const unsigned int idx) {
    if (isOk()) {
        if (idx < values.size()) {
            auto it = std::find_if(values[idx].begin(), values[idx].end(), [](unsigned char c) {
                return (c == ' ' || c == '\0' || c == '\n');
            });
            return std::string(values[idx].begin(), it);
        } else {
            LOG("Reply values are not compatible with the request");
        }
    }
    return std::string();
}
