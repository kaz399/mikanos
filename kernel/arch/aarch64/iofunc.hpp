/**************************************************************/
/**
    @file    iofunc.h

    @brief


    Copyright 2020 Yabe.Kazuhiro

***************************************************************

*/
/**************************************************************/
#ifndef __IOFUNC_H__
#define __IOFUNC_H__

#include <cstdint>

extern "C" {
    void IoOut32(uint16_t addr, uint32_t data);
    uint32_t IoIn32(uint16_t addr);
}

#endif /* __IOFUNC_H__ */

