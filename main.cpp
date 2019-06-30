#define WIDTH 211
#define HEIGHT 57

#include "clac.h"

//TODO: std::thread animation impl
//TODO: std::ostringstream layers

using namespace wok;
using namespace clac;

int main() {
  engine::init("/dev/input/event15");

  log l({math::percent({0, 0}), math::percent({20, 100})}, "Log");
  inventory i({math::percent({80, 0}), math::percent({20, 100})}, "Inventory");

  while (true) {
    event::poll();

    if (event::pressed(KEY_ESC))
      break;

    if (event::pressed(KEY_L))
      l.push(
          "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.");

    if (event::pressed(KEY_I))
      i.push(
          "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.");
  }

  engine::cleanup();

  return 0;
}