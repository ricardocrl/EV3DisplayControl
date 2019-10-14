/*
 * Logging.h
 *
 *  Created on: 14/10/2019
 *      Author: ricardo
 */

#ifndef LOGGING_H_
#define LOGGING_H_

#define LOGGING

#ifdef LOGGING
#include <iostream>
#define LOG(t) std::cout << t << std::endl;
#else
#define LOG(t)
#endif

#endif /* LOGGING_H_ */
