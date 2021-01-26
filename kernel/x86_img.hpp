/**************************************************************/
/**
    @file    x86_img.h

    @brief


    Copyright 2020 Yabe.Kazuhiro

***************************************************************

*/
/**************************************************************/
#ifndef __X86_IMG_H__
#define __X86_IMG_H__

#include <cstdint>

extern "C" {

struct img {
    uint32_t w;
    uint32_t h;
    uint32_t data_size;
    const uint8_t *data;
};

extern const img x86_img;
}
#endif /* __X86_IMG_H__ */

