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
#include <sstream>

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
      T t0, t1;

      inline vec2<T> &operator=(const vec2<T> &v) {
        t0 = v.t0;
        t1 = v.t1;

        return *this;
      }

      inline vec2<T> operator+(const vec2<T> &v) const { return {t0 + v.t0, t1 + v.t1}; }

      inline vec2<T> operator-(const vec2<T> &v) const { return {t0 - v.t0, t1 - v.t1}; }

      inline vec2<T> operator*(const T v) const { return {t0 * v, t1 * v}; }

      inline vec2<T> operator%(const T v) const { return {t0 % v, t1 % v}; }

      inline vec2<T> operator+=(const vec2<T> &v) const { return {t0 + v.t0, t1 + v.t1}; }
    };

    using vec2f = vec2<float>;
    using vec2d = vec2<double>;
    using vec2u = vec2<unsigned>;
    using vec2i = vec2<int>;

    template<typename T>
    struct rect {
      T t0, t1;

      inline rect<T> &operator=(const rect<T> &r) {
        t0 = r.t0;
        t1 = r.t1;

        return *this;
      }
    };

    using rectf = rect<vec2f>;
    using rectd = rect<vec2d>;
    using rectu = rect<vec2u>;
    using recti = rect<vec2i>;

    inline vec2u percent(const vec2u &v, const math::vec2u &t = {WIDTH, HEIGHT}) {
      return {v.t0 * t.t0 / 100, v.t1 * t.t1 / 100};
    }

    inline rectu padding(const rectu &r, const vec2u &m) {
      return {{r.t0.t0 + m.t0,     r.t0.t1 + m.t1},
              {r.t1.t0 - m.t0 * 2, r.t1.t1 - m.t1 * 2}};
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
    std::ostringstream layer;

    void begin_layer() { layer = std::ostringstream{}; }

    void end_layer() { std::cout << layer.str() << std::flush; }

    namespace draw {
      void frame(const math::rectu &window, const std::string &title = "") {
        auto start = window.t0;
        auto end = window.t0 + window.t1;

        for (auto x = start.t0; x < end.t0; x++)
          for (auto y = start.t1; y < end.t1; y++) {
            if (x == start.t0 || x == end.t0 - 1) layer << SET(x, y, "│");
            if (y == start.t1 || y == end.t1 - 1) layer << SET(x, y, "─");

            if (x == start.t0 && y == start.t1) layer << SET(x, y, "┌");
            if (x == end.t0 - 1 && y == start.t1) layer << SET(x, y, "┐");
            if (x == start.t0 && y == end.t1 - 1) layer << SET(x, y, "└");
            if (x == end.t0 - 1 && y == end.t1 - 1) layer << SET(x, y, "┘");
          }

        layer << SET(start.t0 + 1, start.t1, title);
      }

      void text(const math::rectu &window, const std::string &msg) {
        unsigned i = 0, y = 0;

        while (i < msg.size()) {
          auto line = msg.substr(i, window.t1.t0);

          i += window.t1.t0;

          layer << SET(window.t0.t0, window.t0.t1 + y++, line);
        }
      }
    }

    struct view : public std::vector<view> {
      std::string title;

      math::vec2u position;
      math::vec2u size;
      math::vec2u padding;

      math::vec2u offset{};

      view(const std::string &title,
           const math::vec2u &position,
           const math::vec2u &size,
           const math::vec2u &padding) :
          title(title),
          position(position),
          size(size),
          padding(padding) {}

      void draw() const {
        draw::frame({position, size}, title);
        for (auto &v : *this) v.draw();
      }
    };

    template<typename V>
    void align(V &view) {
      view.position = math::percent(view.position) + view.padding;
      view.size = math::percent(view.size) - view.padding * 2;

      auto mod = view.size % 2;

      if (mod.t0) view.size.t0--;
      if (mod.t1) view.size.t1--;
    }

    template<typename View>
    void align_to(View &parent, View &view) {
      view.position = parent.position + math::percent(view.position, parent.size) + view.padding;
      view.size = math::percent(view.size, parent.size) - view.padding * 2;
    }

    template<typename View>
    void stack_horizontal(View &parent, View &view) {
      view.position = parent.position + parent.offset + math::percent(view.position, parent.size) + view.padding;
      view.size = math::percent(view.size, parent.size) - view.padding * 2;
      parent.offset.t0 += view.size.t0;
    }

    template<typename View>
    void stack_vertical(View &parent, View &view) {
      view.position = parent.position + parent.offset + math::percent(view.position, parent.size) + view.padding;
      view.size = math::percent(view.size, parent.size) - view.padding * 2;
      parent.offset.t1 += view.size.t1;
    }
  }

  namespace animator {
    /*std::mutex mutex;

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

    void move_processor(const math::rectu &start, const math::rectu &end) {

    }*/
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