/**
 * @file graphics.cpp
 *
 * 画像描画関連のプログラムを集めたファイル．
 */

#include "memory_manager.hpp"
#include "logger.hpp"

#include "graphics.hpp"

void RGBResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c) {
  auto p = PixelAt(pos);
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c) {
  auto p = PixelAt(pos);
  p[0] = c.b;
  p[1] = c.g;
  p[2] = c.r;
}

PixelColor BlendPixel(const PixelColor& a, const PixelColor& b, const uint8_t alpha) {
  return PixelColor({
      static_cast<uint8_t>((static_cast<uint32_t>(a.r) * (0xff - alpha) + static_cast<uint32_t>(b.r) * alpha) / 0xff),
      static_cast<uint8_t>((static_cast<uint32_t>(a.g) * (0xff - alpha) + static_cast<uint32_t>(b.g) * alpha) / 0xff),
      static_cast<uint8_t>((static_cast<uint32_t>(a.b) * (0xff - alpha) + static_cast<uint32_t>(b.b) * alpha) / 0xff)
      });
}

void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos,
                   const Vector2D<int>& size, const PixelColor& c) {
  for (int dx = 0; dx < size.x; ++dx) {
    writer.Write(pos + Vector2D<int>{dx, 0}, c);
    writer.Write(pos + Vector2D<int>{dx, size.y - 1}, c);
  }
  for (int dy = 1; dy < size.y - 1; ++dy) {
    writer.Write(pos + Vector2D<int>{0, dy}, c);
    writer.Write(pos + Vector2D<int>{size.x - 1, dy}, c);
  }
}

void FillRectangle(PixelWriter& writer, const Vector2D<int>& pos,
                   const Vector2D<int>& size, const PixelColor& c) {
  const auto [ test_pixel, err ]  = GetDesktopPixel(Vector2D<int>{0, 0});

  if (err || c.a == 0xff) {
    for (int dy = 0; dy < size.y; ++dy) {
      for (int dx = 0; dx < size.x; ++dx) {
        writer.Write(pos + Vector2D<int>{dx, dy}, c);
      }
    }
  } else {
    for (int dy = 0; dy < size.y; ++dy) {
      for (int dx = 0; dx < size.x; ++dx) {
        const auto [ desktop_pixel, err ] = GetDesktopPixel(pos + Vector2D<int>{dx, dy});
        if (err) {
          writer.Write(pos + Vector2D<int>{dx, dy}, c);
        } else {
          writer.Write(pos + Vector2D<int>{dx, dy}, BlendPixel(desktop_pixel, c, c.a));
        }
      }
    }
  }
}

PixelColor *desktop_image_buffer = 0;

bool AllocDesktopImageBuffer(PixelWriter& writer) {
  const auto width = writer.Width();
  const auto height = writer.Height();

  size_t desktop_pages = ((height * width * sizeof(PixelColor)) / kBytesPerFrame) + 1;

  auto [ dtbuf, err ] = memory_manager->Allocate(desktop_pages);
  if (err) {
    Log(kError, "failed to allocate desktop image buffer: %s\n", err.Name());
    return false;
  }
  desktop_image_buffer = reinterpret_cast<PixelColor *>(dtbuf.Frame());

  for (int i = 0; i < (height * width); ++i) {
    desktop_image_buffer[i] = kDesktopBGColor;
  }
  return true;
}

static void FillDesktopImage(PixelWriter& writer) {
  const auto width = writer.Width();
  const auto height = writer.Height();
  for (int dy = 0; dy < height; ++dy) {
    for (int dx = 0; dx < width; ++dx) {
      writer.Write(Vector2D<int>{dx, dy}, desktop_image_buffer[(dy * width) + dx]);
    }
  }
}

WithError<PixelColor> GetDesktopPixel(Vector2D<int> pos) {
  PixelColor null_pixel = PixelColor({0x00, 0x00, 0x00});
  auto desktop_size = ScreenSize();
  if (! desktop_image_buffer) {
    return { null_pixel, MAKE_ERROR(Error::kNullPtr) };
  }
  if (pos.x >= desktop_size.x || pos.y >= desktop_size.y) {
    return { null_pixel, MAKE_ERROR(Error::kInvalidFormat) };
  }
  return {
    desktop_image_buffer[(pos.y * desktop_size.x) + pos.x],
    MAKE_ERROR(Error::kSuccess),
  };
}

Error SetDesktopPixel(Vector2D<int> pos, PixelColor& c) {
  auto desktop_size = ScreenSize();
  if (! desktop_image_buffer) {
    return MAKE_ERROR(Error::kNullPtr);
  }
  if (pos.x >= desktop_size.x || pos.y >= desktop_size.y) {
    return MAKE_ERROR(Error::kInvalidFormat);
  }
  desktop_image_buffer[(pos.y * desktop_size.x) + pos.x] = c;
  return MAKE_ERROR(Error::kSuccess);
}

void DrawDesktop(PixelWriter& writer) {
  const auto width = writer.Width();
  const auto height = writer.Height();
  if (desktop_image_buffer) {
    Log(kInfo, "draw background image\n");
    FillDesktopImage(writer);
  } else {
    Log(kInfo, "Fill rectangle\n");
    FillRectangle(writer,
        {0, 0},
        {width, height - 50},
        kDesktopBGColor);
  }
  FillRectangle(writer,
                {0, height - 50},
                {width, 50},
                {1, 8, 17});
  FillRectangle(writer,
                {0, height - 50},
                {width / 5, 50},
                {80, 80, 80});
  DrawRectangle(writer,
                {10, height - 40},
                {30, 30},
                {160, 160, 160});
}

FrameBufferConfig screen_config;
PixelWriter* screen_writer;

Vector2D<int> ScreenSize() {
  return {
    static_cast<int>(screen_config.horizontal_resolution),
    static_cast<int>(screen_config.vertical_resolution)
  };
}

namespace {
  char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
}

void InitializeGraphics(const FrameBufferConfig& screen_config) {
  ::screen_config = screen_config;

  switch (screen_config.pixel_format) {
    case kPixelRGBResv8BitPerColor:
      ::screen_writer = new(pixel_writer_buf)
        RGBResv8BitPerColorPixelWriter{screen_config};
      break;
    case kPixelBGRResv8BitPerColor:
      ::screen_writer = new(pixel_writer_buf)
        BGRResv8BitPerColorPixelWriter{screen_config};
      break;
    default:
      exit(1);
  }

  DrawDesktop(*screen_writer);
}
