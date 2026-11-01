#ifndef XPP_XGUI_WINDOW_HPP
#define XPP_XGUI_WINDOW_HPP

#include "xgui.hpp"

namespace xpp::xgui {
	struct window_t {
		bool visible = true;
		bool visible_in_menu = true;
		std::string name = "Unknown";
		std::string under_menu = "View";

		window_t(std::string _name = "Unknown", bool _visible_in_menu = true)
			: name(std::move(_name)), visible_in_menu(_visible_in_menu) {}
		~window_t(void) {}
		virtual void draw(void) = 0;
	};
}

#endif
