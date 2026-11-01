#ifndef XPP_XGUI_CONSOLE_HPP
#define XPP_XGUI_CONSOLE_HPP

#include "window.hpp"

namespace xpp::xgui {
	struct console_w : window_t {
		ImGuiTextFilter filter;
		char cmdbuf[256];

		console_w(void);
		~console_w(void);
		void draw(void);
	};
}

#endif
