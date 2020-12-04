extern "C" unsigned int KernelMain() {
  volatile unsigned int count = 0;
  for (count = 0; count < 0x10100; ++count) {

//      __asm__("hlt");
  }
  // while (1) __asm__("hlt");
  return count;
}
