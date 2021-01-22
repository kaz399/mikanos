/**
 * @file main.cpp
 *
 * カーネル本体のプログラムを書いたファイル．
 */

#include <cstdint>
#include <cstddef>

// #@@range_begin(includes)
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"

#include "x86_img.hpp"
// #@@range_end(includes)

void* operator new(size_t size, void* buf) {
  return buf;
}

void operator delete(void* obj) noexcept {
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

class Console
{
    private:
        PixelWriter *writer_;
        int x_;
        int y_;
        const int left_margine = 10;
        const int top_margine = 10;
        const int line_spacing = 4;
        const int letter_spacing = 2;
        const int char_width = 8;
        const int char_height = 16;

    public:
        Console(PixelWriter *w): writer_(w), x_(0), y_(0) {
        }

        int puts(const char *str) {
            if (writer_ == 0) {
                return -1;
            }
            while (*str != '\0') {
                WriteAscii(*writer_,
                        left_margine + ((char_width + letter_spacing) * x_),
                        top_margine + ((char_height + line_spacing) * y_),
                        *str,
                        {0, 0, 0});
                x_++;
                str++;
            }
            x_ = 0;
            y_++;
            return y_;
        }
};

char console_instance[sizeof(Console)];
Console *cnsl;

extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config) {
  switch (frame_buffer_config.pixel_format) {
    case kPixelRGBResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf)
        RGBResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
    case kPixelBGRResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf)
        BGRResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
  }

  for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
    for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
        uint8_t r = x % 256;
      pixel_writer->Write(x, y, {r, 255, 255});
    }
  }
  const uint8_t *data = x86_img.data;
  const int offset_x = (frame_buffer_config.horizontal_resolution - x86_img.w) / 2;
  const int offset_y = (frame_buffer_config.vertical_resolution - x86_img.h) / 2;
  for (int y = 0; y < x86_img.h; ++y) {
    for (int x = 0; x < x86_img.w; ++x) {
      uint8_t b = data[0];
      uint8_t g = data[1];
      uint8_t r = data[2];
      data += 4;
      pixel_writer->Write(x + offset_x, y + offset_y, {r, g, b});
    }
  }

  // #@@range_begin(write_fonts)
  int i = 0;
  for (char c = '!'; c <= '~'; ++c, ++i) {
    WriteAscii(*pixel_writer, 8 * i, 70, c, {0, 0, 0});
  }
  cnsl = new(console_instance) Console(pixel_writer);
  cnsl->puts("Hello MikanOS!");
  cnsl->puts("Welcome to the OS development world!!");

#ifdef MIKANOS_AARCH64
  while (1) __asm__("wfi");
#endif

#ifdef MIKANOS_X64
  // #@@range_end(write_fonts)
  while (1) __asm__("hlt");
#endif
}
