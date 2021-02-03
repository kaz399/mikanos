#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <tuple>
#include "../syscall.h"

#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"

std::tuple<int, uint8_t*, size_t> MapFile(const char* filepath) {
  SyscallResult res = SyscallOpenFile(filepath, O_RDONLY);
  if (res.error) {
    fprintf(stderr, "%s: %s\n", strerror(res.error), filepath);
    exit(1);
  }

  const int fd = res.value;
  size_t filesize;
  res = SyscallMapFile(fd, &filesize, 0);
  if (res.error) {
    fprintf(stderr, "%s\n", strerror(res.error));
    exit(1);
  }

  return {fd, reinterpret_cast<uint8_t*>(res.value), filesize};
}

void WaitEvent() {
  AppEvent events[1];
  while (true) {
    auto [ n, err ] = SyscallReadEvent(events, 1);
    if (err) {
      fprintf(stderr, "ReadEvent failed: %s\n", strerror(err));
      return;
    }
    if (events[0].type == AppEvent::kQuit) {
      return;
    }
  }
}

uint32_t GetColorRGB(unsigned char* image_data) {
  return static_cast<uint32_t>(image_data[0]) << 16 |
         static_cast<uint32_t>(image_data[1]) << 8 |
         static_cast<uint32_t>(image_data[2]);
}

uint32_t GetColorGray(unsigned char* image_data) {
  const uint32_t gray = image_data[0];
  return gray << 16 | gray << 8 | gray;
}

extern "C" void main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    exit(1);
  }

  int width, height, bytes_per_pixel;
  const char* filepath = argv[1];
  const auto [ fd, content, filesize ] = MapFile(filepath);

  unsigned char* image_data = stbi_load_from_memory(
      content, filesize, &width, &height, &bytes_per_pixel, 0);
  if (image_data == nullptr) {
    fprintf(stderr, "failed to load image: %s\n", stbi_failure_reason());
    exit(1);
  }

  fprintf(stderr, "%dx%d, %d bytes/pixel\n", width, height, bytes_per_pixel);
  auto get_color = GetColorRGB;
  if (bytes_per_pixel <= 2) {
    get_color = GetColorGray;
  }

  SyscallResult result;
  int desktop_width = 0;
  int desktop_height = 0;

  result = SyscallGetDesktopWidth(&desktop_width);
  if (result.error) {
      fprintf(stderr, "Error: failed to get desktop width:%u\n", result.error);
  }

  result = SyscallGetDesktopHeight(&desktop_height);
  if (result.error) {
      fprintf(stderr, "Error: failed to get desktop height:%u\n", result.error);
  }

  printf("Desktop size [%d x %d]\n", desktop_width, desktop_height);

  for (int y = 0; y < desktop_height; ++y) {
    int image_y = y % height;
    for (int x = 0; x < desktop_width; ++x) {
      int image_x = x % width;
      uint32_t c = get_color(&image_data[bytes_per_pixel * (image_y * width + image_x)]);
      SyscallSetDesktopPixel(x, y, c);
    }
  }

  fprintf(stderr, "redraw bg\n");
  SyscallRedrawDesktop();

  exit(0);
}
