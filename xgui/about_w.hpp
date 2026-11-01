#ifndef XPP_XGUI_ABOUT_HPP
#define XPP_XGUI_ABOUT_HPP

#include "window.hpp"

namespace xpp::xgui {
	struct about_w : window_t {
		std::string renderer = "Unknown";
		std::string lua_version = "Unknown";

		about_w(void);
		~about_w(void);
		void draw(void);
	};
}

#endif
