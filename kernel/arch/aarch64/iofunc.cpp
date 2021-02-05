/**************************************************************/
/**
    @file    iofunc.c

    @brief


    Copyright 2020 Yabe.Kazuhiro

***************************************************************

*/
/**************************************************************/

#include <cstdint>

#include "iofunc.hpp"

extern int printk(const char* format, ...);

extern "C" {
    static const uint64_t PcieIoBaseAddress = 0x3eff0000;

    void IoOut32(uint16_t addr, uint32_t data)
    {
        // write_memory_barrier();
        volatile uint32_t *mmio_addr = (volatile uint32_t *)(PcieIoBaseAddress | addr);
        *mmio_addr = data;
    }

    uint32_t IoIn32(uint16_t addr)
    {
        volatile uint32_t *mmio_addr = (volatile uint32_t *)(PcieIoBaseAddress | addr);
        uint32_t data = *mmio_addr;
        // read_memory_barrier();
        return data;
    }
}

