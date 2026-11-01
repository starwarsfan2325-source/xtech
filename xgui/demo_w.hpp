#ifndef XPP_XGUI_DEMO_HPP
#define XPP_XGUI_DEMO_HPP

#include "window.hpp"

namespace xpp::xgui {
	struct demo_w : window_t {
		demo_w(void);
		~demo_w(void);
		void draw(void);
	};
}

#endif
