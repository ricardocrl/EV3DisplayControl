/*
 * DirectCmdReply.h
 *
 *  Created on: 10/10/2017
 *      Author: roliveira
 */

#ifndef DIRECTCMDREPLY_H_
#define DIRECTCMDREPLY_H_

#include "Logging.h"

#include <vector>
#include <cstddef>
#include <cstring>

class DirectCmdReply {
public:
    DirectCmdReply() : success(false) {};
    DirectCmdReply(unsigned char * buf, const size_t size, const std::vector<std::size_t>& returnSizes);

    template <typename T>
    T get(const unsigned int idx) {
        T ret = 0;
        if (isOk()) {
            if (idx < values.size() && sizeof(T) <= values[idx].size()) {
                memcpy(&ret, values[idx].data(), sizeof(T));
            } else {
                LOG("Reply values are not compatible with the request");
            }
        }
        return ret;
    }

    bool isOk() const {
        return success;
    }

private:
    bool success;
    std::vector<std::vector<unsigned char>> values;

    friend class DirectCmdWrapper;
};

template <> std::string DirectCmdReply::get<std::string>(const unsigned int idx);

#endif /* DIRECTCMDREPLY_H_ */
