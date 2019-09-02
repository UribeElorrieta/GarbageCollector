#include "LeakTester.h"
#include "gc_pointer.h"

int main() {
  Pointer<int> p = new int(19);
  Pointer<int> c = new int(35);
  p = new int(21);
  p = new int(28);
  c = p;
  return 0;
}
