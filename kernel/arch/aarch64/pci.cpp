/**
 * @file pci.cpp
 *
 * PCI バス制御のプログラムを集めたファイル．
 */

#include "pci.hpp"

#include "iofunc.hpp"

extern int printk(const char* format, ...);

namespace {
  using namespace pci;
#if 0
  // #@@range_begin(make_address)
  /** @brief CONFIG_ADDRESS 用の 32 ビット整数を生成する */
  uint32_t MakeAddressPci(uint8_t bus, uint8_t device,
                       uint8_t function, uint8_t reg_addr) {
    auto shl = [](uint32_t x, unsigned int bits) {
        return x << bits;
    };

    return shl(1, 31)  // enable bit
        | shl(bus, 16)
        | shl(device, 11)
        | shl(function, 8)
        | (reg_addr & 0xfcu);
  }
  // #@@range_end(make_address)
#endif
  // address for ECAM
  pci_adrs_t MakeAddress(uint64_t base, uint8_t bus, uint8_t device,
                       uint8_t function, uint16_t reg_addr) {
    auto shl = [](uint64_t x, unsigned int bits) {
        return x << bits;
    };

    pci_adrs_t adrs = base
        | shl((bus & 0xffu), 20)
        | shl((device & 0x1fu), 15)
        | shl((function & 0x07u), 12)
        | (reg_addr & 0xfffu);
    //printk("PCI ADRS:%0p\n", adrs);
    return adrs;
  }

  // #@@range_begin(add_device)
  /** @brief devices[num_device] に情報を書き込み num_device をインクリメントする． */
  Error AddDevice(uint8_t bus, uint8_t device,
                  uint8_t function, uint8_t header_type) {
    if (num_device == devices.size()) {
      return Error::kFull;
    }

    devices[num_device] = Device{bus, device, function, header_type};
    ++num_device;
    return Error::kSuccess;
  }
  // #@@range_end(add_device)

  Error ScanBus(uint8_t bus);

  // #@@range_begin(scan_function)
  /** @brief 指定のファンクションを devices に追加する．
   * もし PCI-PCI ブリッジなら，セカンダリバスに対し ScanBus を実行する
   */
  Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function) {
    auto header_type = ReadHeaderType(bus, device, function);
    if (auto err = AddDevice(bus, device, function, header_type)) {
      return err;
    }

    auto class_code = ReadClassCode(bus, device, function);
    uint8_t base = (class_code >> 24) & 0xffu;
    uint8_t sub = (class_code >> 16) & 0xffu;

    if (base == 0x06u && sub == 0x04u) {
      // standard PCI-PCI bridge
      auto bus_numbers = ReadBusNumbers(bus, device, function);
      uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
      return ScanBus(secondary_bus);
    }

    return Error::kSuccess;
  }
  // #@@range_end(scan_function)

  // #@@range_begin(scan_device)
  /** @brief 指定のデバイス番号の各ファンクションをスキャンする．
   * 有効なファンクションを見つけたら ScanFunction を実行する．
   */
  Error ScanDevice(uint8_t bus, uint8_t device) {
    if (auto err = ScanFunction(bus, device, 0)) {
      return err;
    }
    if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0))) {
      return Error::kSuccess;
    }

    for (uint8_t function = 1; function < 8; ++function) {
      if (ReadVendorId(bus, device, function) == 0xffffu) {
        continue;
      }
      if (auto err = ScanFunction(bus, device, function)) {
        return err;
      }
    }
    return Error::kSuccess;
  }
  // #@@range_end(scan_device)

  // #@@range_begin(scan_bus)
  /** @brief 指定のバス番号の各デバイスをスキャンする．
   * 有効なデバイスを見つけたら ScanDevice を実行する．
   */
  Error ScanBus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; ++device) {
      if (ReadVendorId(bus, device, 0) == 0xffffu) {
        continue;
      }
      if (auto err = ScanDevice(bus, device)) {
        return err;
      }
    }
    return Error::kSuccess;
  }
  // #@@range_end(scan_bus)
}

namespace pci {
  void WriteConfig(pci_adrs_t address, uint32_t data) {
      io_write32(address, data);
  }

  uint32_t ReadConfig(pci_adrs_t address) {
      return io_read32(address);
  }

  uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function) {
      return ReadConfig(MakeAddress(EcamBaseAddress, bus, device, function, 0x00)) & 0xffffu;
  }
  // #@@range_end(config_addr_data)

  uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function) {
      return ReadConfig(MakeAddress(EcamBaseAddress, bus, device, function, 0x00)) >> 16;
  }

  uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
    return (ReadConfig(MakeAddress(EcamBaseAddress, bus, device, function, 0x0c)) >> 16) & 0xffu;
  }

  uint32_t ReadClassCode(uint8_t bus, uint8_t device, uint8_t function) {
    return ReadConfig(MakeAddress(EcamBaseAddress, bus, device, function, 0x08));
  }

  uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function) {
    return ReadConfig(MakeAddress(EcamBaseAddress, bus, device, function, 0x18));
  }

  bool IsSingleFunctionDevice(uint8_t header_type) {
    return (header_type & 0x80u) == 0;
  }

  // #@@range_begin(scan_all_bus)
  Error ScanAllBus() {
    num_device = 0;

    auto header_type = ReadHeaderType(0, 0, 0);
    if (IsSingleFunctionDevice(header_type)) {
      return ScanBus(0);
    }

    for (uint8_t function = 1; function < 8; ++function) {
      if (ReadVendorId(0, 0, function) == 0xffffu) {
        continue;
      }
      if (auto err = ScanBus(function)) {
        return err;
      }
    }
    return Error::kSuccess;
  }
  // #@@range_end(scan_all_bus)
}
