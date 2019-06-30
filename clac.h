#ifndef CLAC_H
#define CLAC_H

#include "wok.h"

using namespace wok;

namespace clac {
  struct message {
    math::rectu _rect;

    std::string _msg;

    message(const math::rectu &rect, const std::string &msg) : _rect(rect), _msg(msg) {
      animator::text({rect.p, {rect.s.x, math::aspect(rect.s.y)}}, msg, 10);
    }
  };

  struct log : public std::vector<message> {
    math::rectu _rect;

    log(const math::rectu &rect, const std::string &title) : _rect(rect) {
      canvas::frame(_rect, title);
    }

    void push(const std::string &msg) {
      emplace_back(math::margin(_rect, {2, 1}), msg);
    }
  };

  struct inventory : public std::vector<message> {
    math::rectu _rect;

    inventory(const math::rectu &rect, const std::string &title) : _rect(rect) {
      canvas::frame(_rect, title);
    }

    void push(const std::string &msg) {
      emplace_back(math::margin(_rect, {2, 1}), msg);
    }
  };
}

#endif