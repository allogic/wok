#define WIDTH 317
#define HEIGHT 79

#include "wok.h"

//TODO: std::thread animation impl

using namespace wok;

template<typename ... Views>
void update(Views &&... views) {
  auto vs = {views...};

  canvas::begin_layer();
  for (auto &v : vs) v.draw();
  canvas::end_layer();
}

int main() {
  engine::init("/dev/input/event15");

  canvas::view log{"Log", {}, {20, 100}, {1, 1}};
  canvas::view inventory{"Inventory", {80, 0}, {20, 100}, {1, 1}};
  canvas::view game{"Game", {20, 0}, {60, 100}, {1, 1}};

  canvas::align(log);
  canvas::align(inventory);
  canvas::align(game);

  update(log, inventory, game);

  while (true) {
    event::poll();

    if (event::pressed(KEY_ESC))
      break;

    if (event::pressed(KEY_L) || event::held(KEY_L)) {
      canvas::view v{"Message", {}, {100, 10}, {1, 1}};
      canvas::stack_vertical(log, v);

      log.emplace_back(v);

      update(log);
    }

    if (event::pressed(KEY_I) || event::held(KEY_I)) {
      canvas::view v{"Item", {}, {100, 20}, {1, 1}};
      canvas::stack_vertical(inventory, v);

      inventory.emplace_back(v);

      update(inventory);
    }
  }

  engine::cleanup();

  return 0;
}