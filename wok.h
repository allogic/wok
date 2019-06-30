#ifndef WOC_H
#define WOC_H

#include <iostream>
#include <vector>
#include <cstdio>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>
#include <functional>

extern "C" {
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
}

#ifndef WIDTH
#define WIDTH 64
#endif

#ifndef HEIGHT
#define HEIGHT 32
#endif

#define FD_GRAB 1
#define FD_UNGRAB 0

#define PRESSED 1
#define HELD 2
#define RELEASED 0

#define CSI "\033["
#define CLS CSI "2J"
#define HCS CSI "?25l"
#define SCS CSI "?25h"

#define MOV(x, y) CSI << std::to_string(y + 1) << ";" << std::to_string(x + 1) << "H"
#define SET(x, y, v) MOV(x, y) << v

namespace wok {
  int kfd;

  namespace instance {
    template<typename T>
    static T *instance = nullptr;

    template<typename T>
    static inline auto &get() {
      return *instance<T>;
    }

    template<typename T, typename ... Args>
    static inline auto &get(Args &&... args) {
      instance<T> = new T(std::forward<Args>(args)...);
      return *instance<T>;
    }
  }

  namespace math {
    template<typename T>
    struct vec2 {
      T x, y;

      inline vec2<T> operator+(const vec2<T> &v) const { return {x + v.x, y + v.y}; }
    };

    using vec2f = vec2<float>;
    using vec2d = vec2<double>;
    using vec2u = vec2<unsigned>;
    using vec2i = vec2<int>;

    template<typename T>
    struct rect {
      T p, s;
    };

    using rectf = rect<vec2f>;
    using rectd = rect<vec2d>;
    using rectu = rect<vec2u>;
    using recti = rect<vec2i>;

    inline vec2u percent(const vec2u &v) { return {(v.x * WIDTH) / 100, (v.y * HEIGHT) / 100}; }

    inline rectu margin(const rectu &r, const vec2u &v) {
      return {{r.p.x + v.x,     r.p.y + v.y},
              {r.s.x - v.x * 2, r.s.y - v.y * 2}};
    }

    inline unsigned aspect(unsigned v) { return v / (WIDTH / HEIGHT); }
  }

  namespace event {
    std::vector<input_event> events;

    void poll() {
      events.clear();

      input_event e{};

      auto n = read(kfd, &e, sizeof(input_event));

      while (n >= 0) {
        events.push_back(e);

        n = read(kfd, &e, sizeof(input_event));
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    bool find(unsigned key, unsigned state) {
      return std::find_if(events.begin(), events.end(), [key, state](const input_event &e) -> bool {
        return e.type == EV_KEY && e.code == key && e.value == state;
      }) != events.end();
    }

    inline bool pressed(unsigned key) { return find(key, PRESSED); }

    inline bool held(unsigned key) { return find(key, HELD); }

    inline bool released(unsigned key) { return find(key, RELEASED); }
  }

  namespace canvas {
    void frame(const math::rectu &rect, const std::string &title = "") {
      auto start = rect.p;
      auto end = rect.p + rect.s;

      for (auto x = start.x; x < end.x; x++)
        for (auto y = start.y; y < end.y; y++) {
          if (x == start.x || x == end.x - 1) std::cout << SET(x, y, "│");
          if (y == start.y || y == end.y - 1) std::cout << SET(x, y, "─");

          if (x == start.x && y == start.y) std::cout << SET(x, y, "┌");
          if (x == end.x - 1 && y == start.y) std::cout << SET(x, y, "┐");
          if (x == start.x && y == end.y - 1) std::cout << SET(x, y, "└");
          if (x == end.x - 1 && y == end.y - 1) std::cout << SET(x, y, "┘");
        }

      std::cout << SET(start.x + 1, start.y, title);

      std::cout << std::flush;
    }

    void text(const math::rectu &rect, const std::string &msg) {
      unsigned i = 0, y = 0;

      while (i < msg.size()) {
        auto line = msg.substr(i, rect.s.x);

        i += rect.s.x;

        std::cout << SET(rect.p.x, rect.p.y + y++, line);
      }

      std::cout << std::flush;
    }
  }

  namespace animator {
    std::mutex mutex;

    void text_processor(const math::rectu &rect, const std::string &msg, unsigned time) {
      unsigned i = 0, y = 0;

      while (i < msg.size()) {
        auto line = msg.substr(i, rect.s.x);

        i += rect.s.x;

        for (auto j = 0; j < line.size(); j++) {
          mutex.lock();

          std::cout << SET(rect.p.x + j, rect.p.y + y, line[j]) << std::flush;

          mutex.unlock();

          std::this_thread::sleep_for(std::chrono::milliseconds(time));
        }

        y++;

        if (y >= rect.p.y + rect.s.y - 1) break;
      }
    }

    void text(const math::rectu &rect, const std::string &msg, unsigned time) {
      std::thread(text_processor, rect, msg, time).detach();
    }
  }

  namespace engine {
    struct termios ptty, ctty;

    void tty() {
      if (tcgetattr(STDIN_FILENO, &ctty) != 0)
        throw std::runtime_error("can not get tty attribute");

      ptty = ctty;

      ctty.c_lflag &= ~(ICANON | ECHO);

      if (tcsetattr(STDIN_FILENO, TCSANOW, &ctty) != 0)
        throw std::runtime_error("can not set tty attribute");
    }

    void keyboard(const char *device) {
      if ((kfd = open(device, O_RDONLY | O_SYNC)) == -1)
        throw std::runtime_error("can not open keyboard fd");

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      if (ioctl(kfd, EVIOCGRAB, FD_GRAB) == -1)
        throw std::runtime_error("can not grab keyboard");

      unsigned flags = fcntl(kfd, F_GETFL, 0);

      flags |= O_NONBLOCK;

      if (fcntl(kfd, F_SETFL, flags) == -1)
        throw std::runtime_error("can not set keyboard flags");
    }

    void cleanup() {
      std::cout << CLS MOV(0, 0) SCS << std::flush;

      if (ioctl(kfd, EVIOCGRAB, FD_UNGRAB) == -1)
        throw std::runtime_error("can not ungrab keyboard");

      close(kfd);

      if (tcsetattr(STDIN_FILENO, TCSANOW, &ptty) != 0)
        throw std::runtime_error("can not cleanup default tty attributes");
    }

    void init(const char *device) {
      tty();
      keyboard(device);

      std::cout << CLS MOV(0, 0) HCS << std::flush;
    }
  }
}

#endif