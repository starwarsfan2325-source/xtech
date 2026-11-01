#ifndef XPP_XGUI_INPUT_HPP
#define XPP_XGUI_INPUT_HPP

#include "window.hpp"

namespace xpp::xgui {
	struct input_w : window_t {
		input_w(void);
		~input_w(void);
		void draw(void);
	};
}

#endif
