#ifndef __LOGGER_HPP_
#define __LOGGER_HPP_

#include <stdio.h>

//#define LOG_ENABLE

#ifdef LOG_ENABLE
    #define LOG_PRINT(...) printf(__VA_ARGS__)
    #define ERROR_PRINT(...) perror(__VA_ARGS__)
#else
    #define LOG_PRINT(...)
    #define ERROR_PRINT(...)
#endif

#endif
